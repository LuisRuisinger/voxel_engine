//
// Created by Luis Ruisinger on 15.03.24.
//
#include <immintrin.h>

#include "Renderer.h"
#include "../Level/Octree/Octree.h"
#include "../util/indices_generator.h"

namespace Renderer {
    Renderer::Renderer(std::shared_ptr<Camera::Perspective::Camera> camera)
        : _camera{std::move(camera)}
        , _vertices{std::make_unique<std::vector<u64>>()}
        , _indices{std::make_unique<std::vector<u32>>()}
        , _width{1800}
        , _height{1200}
        , _projection{}
        , _buffers{}
        , _structures{}
    {
        initGLFW();
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
            _window = glfwCreateWindow(
                    static_cast<i32>(_width),
                    static_cast<i32>(_height), "Farlands", nullptr, nullptr);

            if (!_window)
                throw std::runtime_error{"ERR::RENDERER::INIT::WINDOW"};

            glfwMakeContextCurrent(_window);
            glfwSetWindowUserPointer(_window, static_cast<void *>(this));
            glfwSetFramebufferSizeCallback(_window, [](GLFWwindow *window, i32 width, i32 height) -> void {
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

            glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwSetCursorPosCallback(_window, [](GLFWwindow *window, f64 xpos, f64 ypos) -> void {
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

    auto Renderer::initShaders() -> void {
        _shader = std::make_unique<Shader>("vertex_shader.glsl", "fragment_shader.glsl");

        // setting up uniforms
        _shader->registerUniformLocation("view");
        _shader->registerUniformLocation("projection");
        _shader->registerUniformLocation("worldbase");
    }

    auto Renderer::initPipeline() -> void {
        auto indices_buffer = util::IndicesGenerator<MAX_VERTICES_BUFFER>();
        _indices->insert(_indices->end(), indices_buffer.arr, indices_buffer.end());

        _structures[0] = VoxelStructure::CubeStructure{};

        glEnable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        // VAO generation
        glGenVertexArrays(1, &_buffers[Buffer::VAO]);
        glBindVertexArray(_buffers[Buffer::VAO]);

        // buffer generation
        glGenBuffers(1, &_buffers[Buffer::VBO]);
        glGenBuffers(1, &_buffers[Buffer::EBO]);

        glBindBuffer(GL_ARRAY_BUFFER, _buffers[Buffer::VBO]);
        glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES_BUFFER * sizeof(u64), nullptr, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _buffers[Buffer::EBO]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, _indices->size() * sizeof(u32), _indices->data(), GL_STATIC_DRAW);

        // vertex object space position
        glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(u64), (GLvoid*) 0);
        glEnableVertexAttribArray(0);
        glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(u64), (GLvoid*) sizeof(u32));
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    auto Renderer::addVoxel(u64 packedVoxel) const -> void {
        const constexpr u64 mask = 0x0003FFFFFFFF00FFU;

        // this clears out segments, faces and the unused high 8 bits of the low 16 bits of the low 32 bits
        u16 faces = packedVoxel >> 50;
        auto &ref = _structures[0].mesh();
        packedVoxel &= mask;

        for (size_t i = 0; i < 6; ++i) {
            if (faces & (1 << i)) {
                auto &face = ref[i];

                for (auto vertex : face)
                    _vertices->push_back(vertex | packedVoxel);
            }
        }
    }

    auto Renderer::updateBuffer() -> void {
        if (!_vertices->empty()) {
            glBindBuffer(GL_ARRAY_BUFFER, _buffers[Buffer::VBO]);

            void *bufferData = glMapBufferRange(
                    GL_ARRAY_BUFFER,
                    0,
                    static_cast<GLsizeiptr>(_vertices->size() * sizeof(u64)),
                    GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

            if (!bufferData) {
                int32_t err = glGetError();
                //throw std::runtime_error{"ERR::RENDERER::UPDATEBUFFER::BUFFERHANDLE::" + std::to_string(err) + "\n"};
            }
            else {
                __builtin_memcpy(bufferData, _vertices->data(), _vertices->size() * sizeof(u64));
                glUnmapBuffer(GL_ARRAY_BUFFER);
            }
        }
    }

    auto Renderer::updateGlobalBase(vec2f base) -> void {
        _shader->setVec2("worldbase", base);
    }

    auto Renderer::draw(u32 texture) -> void {

        // --------------
        // pipeline reset

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        _shader->use();
        auto view = _camera->GetViewMatrix();

        _shader->setMat4("view", view);
        _shader->setMat4("projection", _projection);

        glBindVertexArray(_buffers[Buffer::VAO]);

        // binding the texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        // TODO: maybe change to vertices->size()
        glDrawElements(GL_TRIANGLES, static_cast<i32>(_vertices->size()) * 2, GL_UNSIGNED_INT, nullptr);

        _vertices->clear();
    }

    auto Renderer::updateProjectionMatrix() -> void {
        _projection = glm::perspective(
                glm::radians(45.0f),
                static_cast<f32>(_width) / static_cast<f32>(_height),
                0.1f,
                static_cast<f32>((RENDER_RADIUS * 2) * CHUNK_SIZE));
    }

    auto Renderer::getCamera() const -> const Camera::Perspective::Camera * {
        return _camera.get();
    }

    auto Renderer::getWindow() const -> const GLFWwindow * {
        return _window;
    }
}