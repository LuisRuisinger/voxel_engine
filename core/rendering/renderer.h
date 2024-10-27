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

#include "../util/defines.h"
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

    template <typename T>
    class Pass {

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
        auto init_geometry_pass() -> void;
        auto init_shading_pass() -> void;
        auto init_atmosphere_pass() -> void;
        auto init_ssao_pass() -> void;
        auto init_ssao_blur_pass() -> void;
        auto init_depth_map_pass() -> void;
        auto init_water_pass() -> void;
        auto init_ssr_pass() -> void;
        auto init_ssr_blur_pass() -> void;

        std::vector<std::pair<RenderType, Renderable<BaseInterface> *>> sub_renderer;

        framebuffer::Framebuffer depth_map_buffer;
        shader::Program depth_map_pass;

        framebuffer::Framebuffer g_buffer;
        shader::Program g_pass;

        framebuffer::Framebuffer atmosphere_buffer;
        skybox_renderer::SkyboxRenderer skybox;

        framebuffer::Framebuffer ssao_buffer;
        shader::Program ssao_pass;

        framebuffer::Framebuffer ssao_blur_buffer;
        shader::Program ssao_blur_pass;

        framebuffer::Framebuffer water_buffer;
        shader::Program water_pass;

        framebuffer::Framebuffer ssr_buffer;
        shader::Program ssr_pass;

        framebuffer::Framebuffer ssr_blur_buffer;
        shader::Program ssr_blur_pass;

        shader::Program lighting_pass;

        GLuint quad_VAO;
        GLuint quad_VBO;
        GLuint ls_matrices_UBO;

        u32 water_normal_tex;
    };
}


#endif //OPENGL_3D_ENGINE_RENDERER_H
