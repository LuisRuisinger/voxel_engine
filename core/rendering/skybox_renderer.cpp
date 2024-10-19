#include "skybox_renderer.h"
#include "../util/player.h"
#include "../util/stb_image.h"
#include "../util/sun.h"
#include "../util/bin_file.h"

#define TEXTURE_DIMENSION 64
#define AZIMUTH_SEGMENTS 16
#define ZENITH_SEGMENTS 16
#define RADIUS (CHUNK_SIZE * RENDER_RADIUS)
#define MIE_TEXTURE "../resources/maps/mie.bin"
#define RAYLEIGH_TEXTURE "../resources/maps/rayleigh.bin"

namespace core::rendering::skybox_renderer {
    auto SkyboxRenderer::init_shader() -> void {
        auto res = this->shader.init(
                shader::Shader<shader::VERTEX_SHADER>("atmosphere_pass/vertex_shader.glsl"),
                shader::Shader<shader::FRAGMENT_SHADER>("atmosphere_pass/fragment_shader.glsl"));

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

                util::push_back(vertices, x, y, z);
            }
        }

        auto sum = AZIMUTH_SEGMENTS * ZENITH_SEGMENTS;
        
        for (auto i = 0; i < AZIMUTH_SEGMENTS; ++i) {
            auto row = i * ZENITH_SEGMENTS;
            auto next_row = (i + 1) * AZIMUTH_SEGMENTS;

            // downwards facing triangle
            for (auto j = 1; j < ZENITH_SEGMENTS - 1; ++j) {
                auto i1 = row + j;
                auto i2 = row + j + 1;
                auto i3 = (next_row + j) % sum;

                util::push_back(indices, i1, i2, i3);
            }

            // upwards facing triangle
            for (auto j = 0; j < ZENITH_SEGMENTS - 1; ++j) {
                auto i1 = row + j + 1;
                auto i2 = (next_row + j + 1) % sum;
                auto i3 = (next_row + j) % sum;

                util::push_back(indices, i1, i2, i3);
            }
        }

        this->indices_amount = indices.size();

        OPENGL_VERIFY(glGenVertexArrays(1, &this->VAO));
        OPENGL_VERIFY(glBindVertexArray(this->VAO));

        OPENGL_VERIFY(glGenBuffers(1, &this->VBO));
        OPENGL_VERIFY(glBindBuffer(GL_ARRAY_BUFFER, this->VBO));
        OPENGL_VERIFY(glBufferData(
                GL_ARRAY_BUFFER,
                vertices.size() * sizeof(decltype(vertices)::value_type),
                vertices.data(),
                GL_STATIC_DRAW));

        OPENGL_VERIFY(glGenBuffers(1, &this->EBO));
        OPENGL_VERIFY(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO));
        OPENGL_VERIFY(glBufferData(
                GL_ELEMENT_ARRAY_BUFFER,
                indices.size() * sizeof(decltype(indices)::value_type),
                indices.data(),
                GL_STATIC_DRAW));

        auto stride = sizeof(decltype(vertices)::value_type) * 3;
        OPENGL_VERIFY(glVertexAttribPointer(0, 3, GL_FLOAT, false, stride, 0));
        OPENGL_VERIFY(glEnableVertexAttribArray(0));

        OPENGL_VERIFY(glBindVertexArray(0));
        OPENGL_VERIFY(glBindBuffer(GL_ARRAY_BUFFER, 0));
        OPENGL_VERIFY(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));

        // textures
        auto file_read = [](std::string file_name) -> u32 {
            auto file = util::bin_file::File<f32> {
                file_name, TEXTURE_DIMENSION, TEXTURE_DIMENSION, 3, true
            };

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
                            file.buffer()));

            OPENGL_VERIFY(glBindTexture(GL_TEXTURE_2D, 0));
            return texture;
        };

        this->ray_tex = file_read("../resources/maps/rayleigh.bin");
        this->mie_tex = file_read("../resources/maps/mie.bin");
    }

    auto SkyboxRenderer::prepare_frame(state::State &state) -> void {}

    auto SkyboxRenderer::frame(
            state::State &state,
            glm::mat4 &view,
            glm::mat4 &projection) -> void {
        auto camera = state.player.get_camera().get_position();

        this->shader.use();
        OPENGL_VERIFY(glBindVertexArray(this->VAO));

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

        OPENGL_VERIFY(glActiveTexture(GL_TEXTURE0));
        OPENGL_VERIFY(glBindTexture(GL_TEXTURE_2D, this->ray_tex));

        OPENGL_VERIFY(glActiveTexture(GL_TEXTURE1));
        OPENGL_VERIFY(glBindTexture(GL_TEXTURE_2D, this->mie_tex));

        OPENGL_VERIFY(glDrawElements(
                GL_TRIANGLES, this->indices_amount, GL_UNSIGNED_INT, nullptr));
    }
}

#undef TEXTURE_DIMENSION