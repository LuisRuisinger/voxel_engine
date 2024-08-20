//
// Created by Luis Ruisinger on 15.03.24.
//

#include <algorithm>
#include <iostream>

#include "renderer.h"
#include "../Level/Octree/octree.h"
#include "../util/indices_generator.h"

namespace core::rendering {

    const constexpr size_t max_copyable_elements           = MAX_VERTICES_BUFFER * sizeof(u64) / sizeof(VERTEX);
    const constexpr size_t max_displayable_elements_scalar = sizeof(VERTEX) / sizeof(u64) * 1.5;

    Renderer::Renderer(std::shared_ptr<camera::perspective::Camera> camera)
        : _camera{std::move(camera)}
        , _vertices{0}
        , _indices{}
        , _width{1920}
        , _height{1080}
        , _projection{}
        , _buffers{}
    {
        initGLFW();
        initImGui();
        initShaders();
        initPipeline();

        updateProjectionMatrix();
    }

    auto Renderer::initGLFW() -> void {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

        try {
            this->_window = glfwCreateWindow(
                    static_cast<i32>(this->_width),
                    static_cast<i32>(this->_height), "Farlands", nullptr, nullptr);

            if (!this->_window)
                throw std::runtime_error{"ERR::RENDERER::INIT::WINDOW"};

            glfwMakeContextCurrent(this->_window);
            glfwSetWindowUserPointer(this->_window, static_cast<void *>(this));
            glfwSetFramebufferSizeCallback(this->_window, [](GLFWwindow *window, i32 width, i32 height) -> void {
                auto self = static_cast<Renderer *>(glfwGetWindowUserPointer(window));

                self->_width  = width;
                self->_height = height;

                glViewport(0, 0, static_cast<i32>(self->_width), static_cast<i32>(self->_height));

                self->updateProjectionMatrix();
                self->_camera->setFrustumAspect(static_cast<f32>(self->_width) / static_cast<f32>(self->_height));
            });

            // vsync
            glfwSwapInterval(0);

            // glad: load all OpenGL function pointers
            if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
                throw std::runtime_error{"ERR::RENDERER::INIT::GLAD"};

            glfwSetInputMode(this->_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwSetCursorPosCallback(_window, [](GLFWwindow *window, f64 xpos, f64 ypos) -> void {
                if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)
                    return;

                auto self = static_cast<Renderer *>(glfwGetWindowUserPointer(window));
                self->_camera->ProcessMouseMovement(static_cast<f32>(xpos), static_cast<f32>(ypos));
            });
        }
        catch (std::exception &err) {
            std::cerr << err.what() << std::endl;
            glfwTerminate();

            exit(EXIT_FAILURE);
        }
    }

    auto Renderer::initImGui() -> void {
        interface::init(this->_window);
    }

    auto Renderer::initShaders() -> void {
        this->_shader = std::make_unique<Shader>("vertex_shader.glsl", "fragment_shader.glsl");

        // setting up uniforms
        this->_shader->registerUniformLocation("view");
        this->_shader->registerUniformLocation("projection");
        this->_shader->registerUniformLocation("worldbase");
        this->_shader->registerUniformLocation("render_radius");
    }

    auto Renderer::initPipeline() -> void {
        auto indices_buffer = util::IndicesGenerator<MAX_VERTICES_BUFFER>();
        this->_indices.insert(this->_indices.end(), indices_buffer.arr, indices_buffer.end());

        glEnable(GL_DEPTH_TEST);
        // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        // VAO generation
        glGenVertexArrays(1, &this->_buffers[Buffer::VAO]);
        glBindVertexArray(this->_buffers[Buffer::VAO]);

        // buffer generation
        glGenBuffers(1, &this->_buffers[Buffer::VBO]);
        glGenBuffers(1, &this->_buffers[Buffer::EBO]);

        glBindBuffer(GL_ARRAY_BUFFER, this->_buffers[Buffer::VBO]);
        glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES_BUFFER * sizeof(u64), nullptr, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->_buffers[Buffer::EBO]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->_indices.size() * sizeof(u32), this->_indices.data(), GL_STATIC_DRAW);

        // vertex object space position
        glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(u64), (GLvoid*) 0);
        glEnableVertexAttribArray(0);
        glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(u64), (GLvoid*) sizeof(u32));
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    auto Renderer::prepare_buffer(size_t len) -> void * {
        glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES_BUFFER * sizeof(u64), nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, this->_buffers[Buffer::VBO]);

        void *buf = glMapBufferRange(
                GL_ARRAY_BUFFER,
                0,
                static_cast<GLsizeiptr>(len * sizeof(VERTEX)),
                GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

        if (!buf) {
            int32_t err = glGetError();
            throw std::runtime_error{"ERR::RENDERER::UPDATEBUFFER::BUFFERHANDLE::" + std::to_string(err) + "\n"};
        }

        return buf;
    }

    auto Renderer::updateBuffer(const VERTEX *ptr, size_t len) -> void {
        glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES_BUFFER * sizeof(u64), nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, this->_buffers[Buffer::VBO]);

        void *bufferData = glMapBufferRange(
                GL_ARRAY_BUFFER,
                0,
                static_cast<GLsizeiptr>(len * sizeof(VERTEX)),
                GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

        if (!bufferData) {
            int32_t err = glGetError();
            throw std::runtime_error{"ERR::RENDERER::UPDATEBUFFER::BUFFERHANDLE::" + std::to_string(err) + "\n"};
        }
        else {
            std::memcpy(bufferData, ptr, len * sizeof(VERTEX));
            this->_vertices = len;
            glUnmapBuffer(GL_ARRAY_BUFFER);
        }
    }

    auto Renderer::unmap_buffer(size_t len) -> void {
        glUnmapBuffer(GL_ARRAY_BUFFER);
        this->_vertices = len;
    }

    auto Renderer::prepare_frame() -> void {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        this->_shader->use();
        auto view = this->_camera->GetViewMatrix();

        this->_shader->setMat4("view", view);
        this->_shader->setMat4("projection", this->_projection);

        glBindVertexArray(_buffers[Buffer::VAO]);
    }

    auto Renderer::frame() -> void {
        glDrawElements(GL_TRIANGLES,this-> _vertices * max_displayable_elements_scalar, GL_UNSIGNED_INT, nullptr);
        this->_vertices = 0;
    }

    auto Renderer::updateGlobalBase(glm::vec2 base) -> void {
        this->_shader->setVec2("worldbase", base);
    }

    auto Renderer::updateRenderDistance(u32 distance) -> void {
        this->_shader->setInt("render_radius", distance);
    }

    auto Renderer::flush() -> void {}

    auto Renderer::updateProjectionMatrix() -> void {
        this->_projection = glm::perspective(
                glm::radians(45.0f),
                static_cast<f32>(this->_width) / static_cast<f32>(this->_height),
                0.1f,
                static_cast<f32>((RENDER_RADIUS * 2) * 32.0F));
    }

    auto Renderer::getCamera() const -> const camera::perspective::Camera * {
        return this->_camera.get();
    }

    auto Renderer::getWindow() const -> const GLFWwindow * {
        return this->_window;
    }

    auto Renderer::get_batch_size() const -> u64 {
        return max_copyable_elements;
    }
}