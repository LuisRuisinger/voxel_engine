//
// Created by Luis Ruisinger on 06.03.24.
//

#ifndef OPENGL_3D_ENGINE_RENDERER_H
#define OPENGL_3D_ENGINE_RENDERER_H

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "interface.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../threading/thread_pool.h"

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
        explicit Renderer();

        ~Renderer() = default;

        auto initImGui(GLFWwindow *) -> void;
        auto initShaders() -> void;
        auto initPipeline() -> void;

        auto prepare_buffer(size_t) -> void *;
        auto updateBuffer(const VERTEX *, size_t) -> void;
        auto unmap_buffer(size_t) -> void;
        auto updateProjectionMatrix(i32, i32) -> void;
        auto updateGlobalBase(glm::vec2) -> void;
        auto updateRenderDistance(u32) -> void;

        auto prepare_frame(camera::perspective::Camera &camera) -> void;
        auto frame() -> void;
        auto flush() -> void;

        auto get_batch_size() const -> u64;

    private:

        // general renderer data
        //u32                                           _width;
        //u32                                           _height;
        //GLFWwindow                                   *_window;

        // vertex shader data
        // the chunk positions are compressed into 2 * 6 bit
        glm::mat4               _projection;
        std::unique_ptr<Shader> _shader;

        // dynamic vertex vector - contains the current visible verticies for the hooked camera
        std::atomic_size_t         _vertices;
        std::vector<u32>           _indices;

        // GPU buffers
        GLuint _buffers[Buffer::count];
    };
}


#endif //OPENGL_3D_ENGINE_RENDERER_H
