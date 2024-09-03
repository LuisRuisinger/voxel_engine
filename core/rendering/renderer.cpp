//
// Created by Luis Ruisinger on 15.03.24.
//

#include <algorithm>
#include <iostream>

#include "renderer.h"
#include "../../util/indices_generator.h"

namespace core::rendering::renderer {
    auto Renderer::init_ImGui(GLFWwindow *window) -> void {
        interface::init(window);
    }

    auto Renderer::init_pipeline() -> void {

        glEnable(GL_DEPTH_TEST);
        // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        for (auto &[_, v] : this->sub_renderer)
            v->_crtp_init_shader();
    }

    auto Renderer::prepare_frame(state::State &state) -> void {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (auto &[_, v] : this->sub_renderer)
            v->_crtp_prepare_frame(state);

    }

    auto Renderer::frame(state::State &state) -> void {
        for (auto &[_, v] : this->sub_renderer)
            v->_crtp_frame(state);

        interface::render();
    }

    auto Renderer::get_sub_renderer(RenderType render_type) -> Renderable<BaseInterface> & {
        return *this->sub_renderer[render_type];
    }

    auto Renderer::add_sub_renderer(RenderType type, Renderable<BaseInterface> *renderer) -> void {
        this->sub_renderer[type] = renderer;
    }

    auto Renderer::remove_sub_renderer(RenderType render_type) -> void {
        this->sub_renderer.erase(render_type);
    }
}
