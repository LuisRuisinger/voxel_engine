//
// Created by Luis Ruisinger on 06.03.24.
//

#ifndef OPENGL_3D_ENGINE_RENDERER_H
#define OPENGL_3D_ENGINE_RENDERER_H

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "interface.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "../core/state.h"
#include "../core/rendering/shader.h"
#include "../core/rendering/framebuffer.h"
#include "../core/rendering/skybox_renderer.h"

#include "../util/aliases.h"
#include "../util/camera.h"
#include "../util/renderable.h"

namespace core::rendering::renderer {
    using namespace util::renderable;

    enum RenderType : u32 {
        CHUNK_RENDERER,
        MODEL_RENDERER,
        WATER_RENDERER,
        UI_RENDERER,
        DIAGNOSTICS_RENDERER,
        SKYBOX_RENDERER
    };

    class Renderer {
    public:
        Renderer() =default;
        ~Renderer() =default;

        // renderer init
        auto init_ImGui(GLFWwindow *) -> void;
        auto init_pipeline() -> void;

        // per sub_renderer operations
        auto prepare_frame(state::State &) -> void;
        auto frame(state::State &state) -> void;

        auto add_sub_renderer(RenderType, Renderable<BaseInterface> *) -> void;
        auto get_sub_renderer(RenderType) -> Renderable<BaseInterface> &;
        auto remove_sub_renderer(RenderType) -> void;
        auto resize(i32, i32) -> void;

    private:
        std::unordered_map<
                RenderType,
                Renderable<BaseInterface> *> sub_renderer;

        framebuffer::Framebuffer depth_map_buffer;
        shader::Shader depth_map_pass;

        framebuffer::Framebuffer g_buffer;
        shader::Shader lighting_pass;

        framebuffer::Framebuffer atmosphere_buffer;
        skybox_renderer::SkyboxRenderer skybox;

        framebuffer::Framebuffer ssao_buffer;
        shader::Shader ssao_pass;

        framebuffer::Framebuffer ssao_blur_buffer;
        shader::Shader ssao_blur_pass;

        GLuint quad_VAO;
        GLuint quad_VBO;
        GLuint ssao_noise_texture;
    };
}


#endif //OPENGL_3D_ENGINE_RENDERER_H
