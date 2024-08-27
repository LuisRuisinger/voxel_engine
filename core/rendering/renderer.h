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

#include "../../util/aliases.h"
#include "shader.h"
#include "../../util/camera.h"
#include "../state.h"
#include "../../util/renderable.h"

namespace core::rendering::renderer {
    using namespace util::renderable;

    enum RenderType : u32 {
        CHUNK_RENDERER,
        MODEL_RENDERER,
        WATER_RENDERER,
        UI_RENDERER,
        DIAGNOSTICS_RENDERER
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

    private:
        std::unordered_map<
                RenderType,
                Renderable<BaseInterface> *> sub_renderer;

    };
}


#endif //OPENGL_3D_ENGINE_RENDERER_H
