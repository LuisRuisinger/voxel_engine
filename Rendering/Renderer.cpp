//
// Created by Luis Ruisinger on 15.03.24.
//
#include <immintrin.h>

#include "Renderer.h"
#include "../Level/Quadtree/Quadtree.h"

namespace Renderer {
    Renderer::Renderer(std::shared_ptr<Camera::Camera> camera)
    : camera{std::move(camera)}
    , vertices{std::make_unique<std::vector<Vertex>>()}
    , indices{std::make_unique<std::vector<u32>>()}
    , width{1800}
    , height{1200}
    , projection{glm::perspective(
            glm::radians(45.0f), ((f32) this->width) / ((f32) this->height),
            0.1f, ((f32) (RENDER_RADIUS * 2) * CHUNK_SIZE))}
    {
        initGLFW();
        initShaders();
        initPipeline();
    }

    auto Renderer::initGLFW() -> void {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

        this->window = glfwCreateWindow(static_cast<i32>(this->width),
                                        static_cast<i32>(this->height), "Farlands", nullptr, nullptr);
        if (!window) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();

            exit(EXIT_FAILURE);
        }

        glfwMakeContextCurrent(window);
        glfwSetWindowUserPointer(window, static_cast<void *>(this));
        glfwSetFramebufferSizeCallback(window, [](GLFWwindow *wdw, i32 w, i32 h) -> void {
            auto self = static_cast<Renderer *>(glfwGetWindowUserPointer(wdw));

            self->width  = w;
            self->height = h;

            glViewport(0, 0, static_cast<i32>(self->width), static_cast<i32>(self->height));

            self->updateProjectionMatrix();
            self->camera->setFrustumAspect(((f32) self->width) / ((f32) self->height));
        });

        glfwSwapInterval(0);

        // glad: load all OpenGL function pointers
        // ---------------------------------------

        if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
            std::cerr << "Failed to initialize GLAD" << std::endl;
            glfwTerminate();

            exit(EXIT_FAILURE);
        }

        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetCursorPosCallback(window, [](GLFWwindow *wdw, f64 xpos, f64 ypos) -> void {
            auto self = static_cast<Renderer *>(glfwGetWindowUserPointer(wdw));
            self->camera->ProcessMouseMovement(static_cast<f32>(xpos), static_cast<f32>(ypos));
        });
    }

    auto Renderer::initShaders() -> void {
        this->shader = std::make_unique<Shader>("../shaders/vertex_shader.glsl", "../shaders/fragment_shader.glsl");
    }

    auto Renderer::initPipeline() -> void {
        auto indices_buffer = IndicesGenerator<MAX_VERTICES_BUFFER>();
        this->indices->insert(this->indices->end(), indices_buffer.arr, indices_buffer.end());

        this->structures[0] = VoxelStructure::CubeStructure {};

        glEnable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        glGenVertexArrays(1, &this->VAO);
        glBindVertexArray(this->VAO);

        glGenBuffers(1, &this->VBO);
        glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
        glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES_BUFFER * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

        glGenBuffers(1, &this->EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.get()->size() * sizeof(uint32_t), &(this->indices->at(0)), GL_STATIC_DRAW);

        // vertex object space position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
        glEnableVertexAttribArray(0);
    }

    auto Renderer::addVoxel(const BoundingVolume *bVol) const -> void {
        const auto &mesh = structures[bVol->voxelID & UINT8_MAX].structure();

        for (u8 i = 0; i < 6; ++i) {
            if (bVol->voxelID & (1 << (i + 10))) {
                const auto &currentMesh = mesh[i];
                const auto &verts = currentMesh.vertices;

                this->vertices->reserve(this->vertices->size() + verts.size());

                for (const auto &vert : verts) {
                    this->vertices->push_back({
                        .pos = bVol->position + vert.position * ((f32) bVol->scale),
                    });
                }
            }
        }
    }

    auto Renderer::updateBuffer() -> void {
        if (this->vertices->empty())
            return;

        glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
        void *bufferData =
                glMapBufferRange(GL_ARRAY_BUFFER,
                                 0,
                                 this->vertices->size() * sizeof(Vertex),
                                 GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

        __builtin_memcpy(bufferData, this->vertices->data(), this->vertices->size() * sizeof(Vertex));
        glUnmapBuffer(GL_ARRAY_BUFFER);
    }

    auto Renderer::draw(u32 texture) -> void {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader->use();

        auto view          = this->camera->GetViewMatrix();
        auto viewLoc       = glGetUniformLocation(shader->ID, "view");
        auto projectionLoc = glGetUniformLocation(shader->ID, "projection");

        glUniformMatrix4fv(viewLoc,       1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(this->projection));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(this->VAO);
        glDrawElements(GL_TRIANGLES, static_cast<int>(this->indices->size()), GL_UNSIGNED_INT, nullptr);

        this->vertices->clear();
    }

    auto Renderer::updateProjectionMatrix() -> void {
        this->projection =
                glm::perspective(glm::radians(45.0f),
                                 ((f32) this->width) / ((f32) this->height),
                                 0.1f,
                                 ((f32) (RENDER_RADIUS * 2) * CHUNK_SIZE));
    }

    auto Renderer::getCamera() const -> const Camera::Camera * {
        return this->camera.get();
    }

    auto Renderer::getWindow() const -> const GLFWwindow * {
        return this->window;
    }
}