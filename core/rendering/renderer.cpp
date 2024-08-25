//
// Created by Luis Ruisinger on 15.03.24.
//

#include <algorithm>
#include <iostream>

#include "renderer.h"
#include "../../util/indices_generator.h"

namespace core::rendering::renderer {

    const constexpr size_t max_copyable_elements = MAX_VERTICES_BUFFER * sizeof(u64) / sizeof(VERTEX);
    const constexpr size_t max_displayable_elements_scalar = sizeof(VERTEX) / sizeof(u64) * 1.5;

    Renderer::Renderer()
        : vertex_buffer_size { 0 }
        , projection_matrix  {   }
        , gpu_buffers        {   }
    {}

    auto Renderer::init_ImGui(GLFWwindow *window) -> void {
        interface::init(window);
    }

    auto Renderer::init_shaders() -> void {
        this->shader = std::make_unique<shader::Shader>(
                "vertex_shader.glsl", "fragment_shader.glsl");

        // setting up uniforms
        this->shader->registerUniformLocation("view");
        this->shader->registerUniformLocation("projection");
        this->shader->registerUniformLocation("worldbase");
        this->shader->registerUniformLocation("render_radius");
    }

    auto Renderer::init_pipeline() -> void {
        auto indices_buffer = util::IndicesGenerator<MAX_VERTICES_BUFFER>();
        this->indices.insert(this->indices.end(), indices_buffer.arr, indices_buffer.end());

        glEnable(GL_DEPTH_TEST);
        // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        // VAO generation
        glGenVertexArrays(1, &this->gpu_buffers[Buffer::VAO]);
        glBindVertexArray(this->gpu_buffers[Buffer::VAO]);

        // buffer generation
        glGenBuffers(1, &this->gpu_buffers[Buffer::VBO]);
        glGenBuffers(1, &this->gpu_buffers[Buffer::EBO]);

        glBindBuffer(GL_ARRAY_BUFFER, this->gpu_buffers[Buffer::VBO]);
        glBufferData(GL_ARRAY_BUFFER,
                     MAX_VERTICES_BUFFER * sizeof(u64),
                     nullptr,
                     GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->gpu_buffers[Buffer::EBO]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     this->indices.size() * sizeof(u32),
                     this->indices.data(),
                     GL_STATIC_DRAW);

        // vertex object space position
        glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(u64), nullptr);
        glEnableVertexAttribArray(0);
        glVertexAttribIPointer(
                1, 1, GL_UNSIGNED_INT, sizeof(u64), reinterpret_cast<GLvoid*>(sizeof(u32)));
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    auto Renderer::prepare_frame(util::camera::Camera &camera) -> void {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        this->shader->use();
        auto view = camera.get_view_matrix();

        this->shader->setMat4("view", view);
        this->shader->setMat4("projection", this->projection_matrix);

        glBindVertexArray(gpu_buffers[Buffer::VAO]);
    }

    auto Renderer::update_buffer(const VERTEX *ptr, size_t len) -> void {
        glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES_BUFFER * sizeof(u64), nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, this->gpu_buffers[Buffer::VBO]);

        void *bufferData =
                glMapBufferRange(GL_ARRAY_BUFFER,
                                 0,
                                 static_cast<GLsizeiptr>(len * sizeof(VERTEX)),
                                 GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

        if (!bufferData) {
            int32_t err = glGetError();
            throw std::runtime_error{
                "ERR::RENDERER::UPDATEBUFFER::BUFFERHANDLE::" + std::to_string(err) + "\n"
            };
        }
        else {
            std::memcpy(bufferData, ptr, len * sizeof(VERTEX));
            this->vertex_buffer_size = len;
            glUnmapBuffer(GL_ARRAY_BUFFER);
        }
    }

    auto Renderer::frame() -> void {
        glDrawElements(GL_TRIANGLES,
                       this-> vertex_buffer_size * max_displayable_elements_scalar,
                       GL_UNSIGNED_INT,
                       nullptr);
        this->vertex_buffer_size = 0;
    }

    auto Renderer::update_current_global_base(glm::vec2 base) -> void {
        this->shader->setVec2("worldbase", base);
    }

    auto Renderer::update_render_distance(u32 distance) -> void {
        this->shader->setInt("render_radius", distance);
    }

    auto Renderer::update_projection_matrix(i32 width, i32 height) -> void {
        this->projection_matrix =
                glm::perspective(glm::radians(45.0f),
                                 static_cast<f32>(width) / static_cast<f32>(height),
                                 0.1f,
                                 static_cast<f32>((RENDER_RADIUS * 2) * 32.0F));
    }

    auto Renderer::get_batch_size() const -> u64 {
        return max_copyable_elements;
    }
}