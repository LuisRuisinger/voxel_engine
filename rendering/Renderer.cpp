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
        , _vertices{std::make_unique<std::vector<Vertex>>()}
        , _indices{std::make_unique<std::vector<u32>>()}
        , _width{1800}
        , _height{1200}
        , _projection{}
        , _chunks{std::make_unique<std::vector<u16>>()}
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

        _window = glfwCreateWindow(
                static_cast<i32>(_width),
                static_cast<i32>(_height), "Farlands", nullptr, nullptr);
        
        if (!_window) {
            std::cerr << "Failed to create GLFW _window" << std::endl;
            glfwTerminate();

            exit(EXIT_FAILURE);
        }

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

        // -----
        // vsync

        glfwSwapInterval(1);

        // glad: load all OpenGL function pointers
        // ---------------------------------------

        if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
            std::cerr << "Failed to initialize GLAD" << std::endl;
            glfwTerminate();

            exit(EXIT_FAILURE);
        }

        glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetCursorPosCallback(_window, [](GLFWwindow *window, f64 xpos, f64 ypos) -> void {
            auto self = static_cast<Renderer *>(glfwGetWindowUserPointer(window));
            self->_camera->ProcessMouseMovement(static_cast<f32>(xpos), static_cast<f32>(ypos));
        });
    }

    auto Renderer::initShaders() -> void {
        _shader = std::make_unique<Shader>("vertex_shader.glsl", "fragment_shader.glsl");

        _shader->registerUniformLocation("view");
        _shader->registerUniformLocation("projection");
        _shader->registerUniformLocation("worldbase");
    }

    auto Renderer::initPipeline() -> void {
        auto indices_buffer = util::IndicesGenerator<MAX_VERTICES_BUFFER>();
        _indices->insert(_indices->end(), indices_buffer.arr, indices_buffer.end());
        
        _structures[0] = VoxelStructure::CubeStructure {};

        glEnable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        // VAO generation
        glGenVertexArrays(1, &_buffers[Buffer::VAO]);
        glBindVertexArray(Buffer::VAO);

        // buffer generation
        glGenBuffers(1, &_buffers[Buffer::DRAW_ID]);
        glGenBuffers(1, &_buffers[Buffer::VBO]);
        glGenBuffers(1, &_buffers[Buffer::EBO]);
        glGenBuffers(1, &_buffers[Buffer::TBO]);

        // draw_ID substitute
        //glBindBuffer(GL_ARRAY_BUFFER, _buffers[Buffer::DRAW_ID]);
        //glVertexAttribIPointer(_buffers[Buffer::DRAW_ID], 1, GL_UNSIGNED_INT, sizeof(glm::uint), 0);
        //glad_glVertexAttribDivisor(_buffers[Buffer::DRAW_ID], 1);
        //glBindBuffer(GL_ARRAY_BUFFER, 0);
        //glEnableVertexAttribArray(_DrawID);


        // remaining buffers
        glBindBuffer(GL_ARRAY_BUFFER, _buffers[Buffer::VBO]);
        glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES_BUFFER * sizeof(*(_vertices->data())), nullptr, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _buffers[Buffer::EBO]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, _indices->size() * sizeof(*(_indices->data())), _indices->data(), GL_STATIC_DRAW);

        glBindBuffer(GL_TEXTURE_BUFFER, _buffers[Buffer::TBO]);
        glBufferData(GL_TEXTURE_BUFFER, MAX_RENDER_VOLUME * sizeof(*(_chunks->data())), nullptr, GL_DYNAMIC_DRAW);

        // vertex object space position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(*(_vertices->data())), nullptr);
        glEnableVertexAttribArray(0);
    }

    auto Renderer::addVoxel(const BoundingVolumeVoxel *voxel) const -> void {
        const auto &mesh = _structures[voxel->_voxelID & UINT8_MAX].structure();

        for (u8 i = 0; i < 6; ++i) {
            if (voxel->_voxelID & (1 << (i + 10))) {
                const auto &currentMesh = mesh[i];
                const auto &verts = currentMesh.vertices;

                _vertices->reserve(_vertices->size() + verts.size());

                for (const auto &vert : verts) {
                    _vertices->push_back({
                        ._pos = voxel->_position + vert.position * ((f32) voxel->_scale),
                    });
                }
            }
        }
    }

    auto Renderer::addChunk(const vec3f &chunkPos) const -> void {

        // ----------------------------------------------------
        // a chunk contains 16 chunksegments -> 4 bits y offset
        // max volume is 64^2 chunks -> 6 bits x and z offset

        u16 x = static_cast<u32>(chunkPos.x) & 0x3F;
        u16 y = static_cast<u32>(chunkPos.y) & 0xF;
        u16 z = static_cast<u32>(chunkPos.z) & 0x3F;

        _chunks->emplace_back((x << 10) | (y << 6) | z);
    }

    auto Renderer::updateBuffer() -> void {
        void *bufferData = nullptr;

        if (!_vertices->empty()) {
            glBindBuffer(GL_ARRAY_BUFFER, _buffers[Buffer::VBO]);
            bufferData = glMapBufferRange(
                    GL_ARRAY_BUFFER,
                    0,
                    static_cast<GLsizeiptr>(_vertices->size() * sizeof(*(_vertices->data()))),
                    GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

            __builtin_memcpy(
                    bufferData,
                    _vertices->data(),
                    _vertices->size() * sizeof(*(_vertices->data())));

            glUnmapBuffer(GL_ARRAY_BUFFER);
        }

        if (!this->_chunks->empty()) {
            glBindBuffer(GL_TEXTURE_BUFFER, _buffers[Buffer::TBO]);
            bufferData = glMapBufferRange(
                    GL_TEXTURE_BUFFER,
                    0,
                    static_cast<GLsizeiptr>(_chunks->size() * sizeof(*(_chunks->data()))),
                    GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

            __builtin_memcpy(
                    bufferData,
                    _chunks->data(),
                    _chunks->size() * sizeof(*(_chunks->data())));

            glUnmapBuffer(GL_TEXTURE_BUFFER);
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

        // -------------------
        // binding the texture

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_R16, _buffers[Buffer::TBO]);

        glDrawElements(GL_TRIANGLES, static_cast<i32>(_indices->size()), GL_UNSIGNED_INT, nullptr);

        // ----------------------------------
        // clear dynamic per render tick data

        _vertices->clear();
        _chunks->clear();
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