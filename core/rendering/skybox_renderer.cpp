#include "skybox_renderer.h"
#include "../util/player.h"
#include "../util/stb_image.h"
#include "../util/sun.h"

#define TEXTURE_DIMENSION 64
#define AZIMUTH_SEGMENTS 32
#define ZENITH_SEGMENTS 32
#define RADIUS (CHUNK_SIZE * RENDER_RADIUS)

namespace core::rendering::skybox_renderer {
    auto SkyboxRenderer::init_shader() -> void {
        auto res = this->shader.init(
                "atmosphere_pass/vertex_shader.glsl",
                "atmosphere_pass/fragment_shader.glsl");

        if (res.isErr()) {
            LOG(util::log::LOG_LEVEL_ERROR, res.unwrapErr());
            std::exit(EXIT_FAILURE);
        }

        this->shader.use();

        // vertex shader uniforms
        this->shader.registerUniformLocation("view");
        this->shader.registerUniformLocation("projection");
        this->shader.registerUniformLocation("camera");

        // fragment shader uniforms
        this->shader.registerUniformLocation("lightDir");
        this->shader.registerUniformLocation("rayleighTexture");
        this->shader.registerUniformLocation("mieTexture");
        this->shader.registerUniformLocation("exposure");
        this->shader.registerUniformLocation("rayleighEnabled");
        this->shader.registerUniformLocation("mieEnabled");
        this->shader.registerUniformLocation("mieG");

        // skydome - a simple sphere
        // source https://www.songho.ca/opengl/gl_sphere.html
        // TODO: put this in a util file
        // TODO: add static renderable as crtp

        std::vector<f32> vertices;
        std::vector<u32> indices;

        // sphere vertices
        for (auto i = 0; i < AZIMUTH_SEGMENTS; ++i) {
            f32 azimuth_rad =
                    (2.0F * M_PI) *
                    (static_cast<f32>(i) / static_cast<f32>(AZIMUTH_SEGMENTS));

            for (auto j = 0; j < ZENITH_SEGMENTS; ++j) {
                f32 zenith_rad = M_PI * (static_cast<f32>(j) / static_cast<f32>(ZENITH_SEGMENTS));

                f32 x = RADIUS * cos(azimuth_rad) * sin(zenith_rad);
                f32 y = RADIUS * sin(azimuth_rad) * sin(zenith_rad);
                f32 z = RADIUS * cos(zenith_rad);

                vertices.push_back(x);
                vertices.push_back(y);
                vertices.push_back(z);
            }
        }

        auto sum = AZIMUTH_SEGMENTS * ZENITH_SEGMENTS;

        // produces indices for 60% of a sphere
        for (auto i = 0; i < AZIMUTH_SEGMENTS; ++i) {
            auto row = i * ZENITH_SEGMENTS;
            auto next_row = (i + 1) * AZIMUTH_SEGMENTS;

            // downwards facing triangle
            for (auto j = 1; j < ZENITH_SEGMENTS * 0.6F; ++j) {
                auto i1 = row + j;
                auto i2 = row + j + 1;
                auto i3 = (next_row + j) % sum;

                indices.push_back(i1);
                indices.push_back(i2);
                indices.push_back(i3);
            }

            // upwards facing triangle
            for (auto j = 0; j < ZENITH_SEGMENTS * 0.6F; ++j) {
                auto i1 = row + j + 1;
                auto i2 = (next_row + j + 1) % sum;
                auto i3 = (next_row + j) % sum;

                indices.push_back(i1);
                indices.push_back(i2);
                indices.push_back(i3);
            }
        }

        this->indices_amount = indices.size();

        glGenVertexArrays(1, &this->VAO);
        glBindVertexArray(this->VAO);

        glGenBuffers(1, &this->VBO);
        glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
        glBufferData(
                GL_ARRAY_BUFFER,
                vertices.size() * sizeof(decltype(vertices)::value_type),
                vertices.data(),
                GL_STATIC_DRAW);


        //
        //
        //

        glGenBuffers(1, &this->EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
        glBufferData(
                GL_ELEMENT_ARRAY_BUFFER,
                indices.size() * sizeof(decltype(indices)::value_type),
                indices.data(),
                GL_STATIC_DRAW);

        //
        //
        //

        LOG("amount vertices", vertices.size());
        LOG("amount indices", indices.size());

        auto stride = sizeof(decltype(vertices)::value_type) * 3;
        glVertexAttribPointer(0, 3, GL_FLOAT, false, stride, 0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // textures
        // TODO: move / opengl class for textures

        auto file_read = [](std::string file_name) -> u32 {
            std::ifstream file { file_name, std::ios::binary };
            if (!file) {
                LOG("Failed to load", file_name);
                std::exit(EXIT_FAILURE);
            }

            file.seekg(0 , std::ios::end);
            u32 file_len = file.tellg();

            file.seekg(0, std::ios::beg);

            auto buffer = new f32[file_len / sizeof(f32)];
            file.read(reinterpret_cast<char *>(buffer), file_len);
            file.close();

            auto swap_buffer = new f32[file_len / sizeof(f32)];

            for (auto i = 0; i < TEXTURE_DIMENSION; ++i) {
                for (auto j = 0; j < TEXTURE_DIMENSION; ++j) {
                    for (auto k = 0; k < 3; ++k) {
                        swap_buffer[(i + j * TEXTURE_DIMENSION) * 3 + k] =
                                buffer[(i + (TEXTURE_DIMENSION - 1 - j) * TEXTURE_DIMENSION) * 3 + k];
                    }
                }
            }

            delete[] buffer;

            // constructing image
            GLuint texture;
            OPENGL_VERIFY(glGenTextures(1, &texture));
            OPENGL_VERIFY(glBindTexture(GL_TEXTURE_2D, texture));

            // Set texture parameters
            OPENGL_VERIFY(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
            OPENGL_VERIFY(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
            OPENGL_VERIFY(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
            OPENGL_VERIFY(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

            OPENGL_VERIFY(
                    glTexImage2D(
                            GL_TEXTURE_2D,
                            0,
                            GL_RGB32F,
                            TEXTURE_DIMENSION,
                            TEXTURE_DIMENSION,
                            0,
                            GL_RGB,
                            GL_FLOAT,
                            swap_buffer));

            OPENGL_VERIFY(glBindTexture(GL_TEXTURE_2D, 0));
            LOG("Successfully read and parsed", file_name);

            delete[] swap_buffer;
            return texture;
        };

        this->ray_tex = file_read("../resources/maps/rayleigh.bin");
        this->mie_tex = file_read("../resources/maps/mie.bin");
    }

    auto SkyboxRenderer::prepare_frame(state::State &state) -> void {
        // TODO: maybe update the sun here
    }

    auto SkyboxRenderer::frame(state::State &state) -> void {
        auto view = state.player.get_camera().get_view_matrix();
        auto projection = state.player.get_camera().get_projection_matrix();
        auto camera = state.player.get_camera().get_position();

        this->shader.use();
        glBindVertexArray(this->VAO);

        // vertex shader uniforms
        this->shader["view"] = view;
        this->shader["projection"] = projection;
        this->shader["camera"] = camera;

        // fragment shader uniforms
        this->shader["lightDir"] = state.sun.get_orientation();
        this->shader["rayleighTexture"] = 0;
        this->shader["mieTexture"] = 1;
        this->shader["exposure"] = 0.9F;
        this->shader["rayleighEnabled"] = true;
        this->shader["mieEnabled"] = true;
        this->shader["mieG"] = 0.85F;

        this->shader.upload_uniforms();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, this->ray_tex);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, this->mie_tex);

        glDrawElements(GL_TRIANGLES,                    // primitive type
                       this->indices_amount,          // # of indices
                       GL_UNSIGNED_INT,                 // data type
                       nullptr);

    }
}

#undef TEXTURE_DIMENSION