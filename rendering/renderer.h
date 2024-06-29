//
// Created by Luis Ruisinger on 06.03.24.
//

#ifndef OPENGL_3D_ENGINE_RENDERER_H
#define OPENGL_3D_ENGINE_RENDERER_H

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "interface.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../threading/thread_pool.h"

#ifdef __AVX2__
#include <immintrin.h>
    #define VERTEX __m256i
#else
    #define VERTEX u64
#endif

#include "../util/aliases.h"
#include "shader.h"
#include "../Level/Model/mesh.h"
#include "../camera/camera.h"
#include "../Level/Model/Types/voxel.h"
#include "../Level/Octree/boundingVolume.h"

#define MAX_VERTICES_BUFFER ((u32) (131072 * 2.5))

namespace core::rendering {

    //
    //
    //

    //
    //
    //

    Enum(Buffer, VAO, VBO, EBO);

    //
    //
    //

    class Renderer {
    public:
        explicit Renderer(std::shared_ptr<camera::perspective::Camera> camera);

        ~Renderer() = default;

        auto initGLFW() -> void;
        auto initImGui() -> void;
        auto initShaders() -> void;
        auto initPipeline() -> void;

        auto prepareBuffer() -> void;
        auto updateBuffer(VERTEX *, size_t) -> void;
        auto updateProjectionMatrix() -> void;
        auto updateGlobalBase(glm::vec2) -> void;
        auto updateRenderDistance(u32) -> void;

        auto prepare_frame() -> void;
        auto frame() -> void;
        auto flush() -> void;

        auto getCamera() const -> const camera::perspective::Camera *;
        auto getWindow() const -> const GLFWwindow *;
        auto get_batch_size() const -> u64;

    private:

        // general renderer data
        u32                                           _width;
        u32                                           _height;
        GLFWwindow                                   *_window;
        std::shared_ptr<camera::perspective::Camera>  _camera;


        // vertex shader data
        // the chunk positions are compressed into 2 * 6 bit
        glm::mat4               _projection;
        std::unique_ptr<Shader> _shader;

        // dynamic vertex vector - contains the current visible verticies for the hooked camera
        size_t                     _vertices;
        std::vector<u32>           _indices;

        // GPU buffers
        GLuint _buffers[Buffer::count];
    };
}


#endif //OPENGL_3D_ENGINE_RENDERER_H
