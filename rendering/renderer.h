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

#define MAX_VERTICES_BUFFER (static_cast<u32>(131072 * 2.5))

namespace core::rendering {
    Enum(Buffer, VAO, VBO, EBO);

    class Renderer {
    public:
        Renderer();
        ~Renderer() = default;

        // renderer init
        auto init_ImGui(GLFWwindow *) -> void;
        auto init_shaders() -> void;
        auto init_pipeline() -> void;

        // per frame pipeline
        auto prepare_frame(camera::perspective::Camera &camera) -> void;
        auto update_buffer(const VERTEX *, size_t) -> void;
        auto frame() -> void;

        // uniforms
        auto update_projection_matrix(i32, i32) -> void;
        auto update_current_global_base(glm::vec2) -> void;
        auto update_render_distance(u32) -> void;

        // getter
        auto get_batch_size() const -> u64;

    private:
        std::unique_ptr<Shader> shader;
        glm::mat4 projection_matrix;

        std::atomic<size_t> vertex_buffer_size;
        std::vector<u32> indices;

        GLuint gpu_buffers[Buffer::count];
    };
}


#endif //OPENGL_3D_ENGINE_RENDERER_H
