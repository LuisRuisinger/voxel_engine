//
// Created by Luis Ruisinger on 15.03.24.
//

#include <algorithm>
#include <iostream>

#include "renderer.h"

#include "../util/sun.h"
#include "../util/indices_generator.h"
#include "../util/player.h"
#include "../util/color.h"

namespace core::rendering::renderer {
    auto Renderer::init_ImGui(GLFWwindow *window) -> void {
        interface::init(window);
    }

    auto Renderer::init_pipeline() -> void {
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        // atmosphere pass
        auto init_atmosphere_pass = [](framebuffer::Framebuffer &target, i32 width, i32 height) {

            // allocate buffers
            target.buffer.resize(1);

            // color + specular color buffer
            glGenTextures(1, &target.buffer[0]);
            glBindTexture(GL_TEXTURE_2D, target.buffer[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target.buffer[0], 0);
        };

        auto destroy_atmosphere_pass = [](framebuffer::Framebuffer &target) {
            glDeleteTextures(1, target.buffer.data());
        };

        this->atmosphere_buffer = { init_atmosphere_pass, destroy_atmosphere_pass };
        this->atmosphere_buffer.bind();

        glEnable(GL_FRAMEBUFFER_SRGB);

        this->skybox.init_shader();

        // lighting pass
        auto res = this->lighting_pass.init(
                "shading_pass/vertex_shader.glsl",
                "shading_pass/fragment_shader.glsl");

        if (res.isErr()) {
            LOG(util::log::LOG_LEVEL_ERROR, res.unwrapErr());
            std::exit(EXIT_FAILURE);
        }

        this->lighting_pass.use();
        this->lighting_pass.registerUniformLocation("g_position");
        this->lighting_pass.registerUniformLocation("g_normal");
        this->lighting_pass.registerUniformLocation("g_albedospec");
        this->lighting_pass.registerUniformLocation("g_atmosphere");

        this->atmosphere_buffer.unbind();

#ifdef SSAO_PASS
        this->lighting_pass.registerUniformLocation("g_ssao");
#endif

        // sun shading attributes
        this->lighting_pass.registerUniformLocation("light_direction");
        this->lighting_pass.registerUniformLocation("view_direction");

        // screen-space quad
        f32 quad_vertices[] = {
                -1.0f,  1.0f,  0.0f, 1.0f,
                -1.0f, -1.0f,  0.0f, 0.0f,
                 1.0f, -1.0f,  1.0f, 0.0f,

                -1.0f,  1.0f,  0.0f, 1.0f,
                 1.0f, -1.0f,  1.0f, 0.0f,
                 1.0f,  1.0f,  1.0f, 1.0f
        };
        
        glGenVertexArrays(1, &this->quad_VAO);
        glBindVertexArray(this->quad_VAO);

        glGenBuffers(1, &this->quad_VBO);
        glBindBuffer(GL_ARRAY_BUFFER, this->quad_VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        // geometry pass
        auto init = [](framebuffer::Framebuffer &target, i32 width, i32 height) {

            // allocate buffers
            target.buffer.resize(4);

            // position color buffer
            glGenTextures(1, &target.buffer[0]);
            glBindTexture(GL_TEXTURE_2D, target.buffer[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target.buffer[0], 0);

            // normal color buffer
            glGenTextures(1, &target.buffer[1]);
            glBindTexture(GL_TEXTURE_2D, target.buffer[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, target.buffer[1], 0);

            // color + specular color buffer
            glGenTextures(1, &target.buffer[2]);
            glBindTexture(GL_TEXTURE_2D, target.buffer[2]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, target.buffer[2], 0);

            // opengl needs to know which color attachments to use for the rendering of
            // this framebuffer
            u32 attachments[4] = {
                    GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2
            };

            glDrawBuffers(3, attachments);

            // depth buffer
            glGenRenderbuffers(1, &target.buffer[3]);
            glBindRenderbuffer(GL_RENDERBUFFER, target.buffer[3]);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, target.buffer[3]);
        };

        auto destroy = [](framebuffer::Framebuffer &target) {
            glDeleteTextures(3, target.buffer.data());
            glDeleteRenderbuffers(1, &target.buffer[3]);
        };

        this->g_buffer = { init, destroy };
        this->g_buffer.bind();
        this->sub_renderer[RenderType::CHUNK_RENDERER]->_crtp_init_shader();
        this->g_buffer.unbind();

#ifdef SSAO_PASS
        auto init_ssao = [](framebuffer::Framebuffer &target, i32 width, i32 height) {

            // allocator buffers
            target.buffer.resize(1);

            // ssao color buffer
            glGenTextures(1, &target.buffer[0]);
            glBindTexture(GL_TEXTURE_2D, target.buffer[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_FLOAT, nullptr);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target.buffer[0], 0);
        };

        auto destroy_ssao = [](framebuffer::Framebuffer &target) {
            glDeleteTextures(1, target.buffer.data());
        };

        this->ssao_buffer = { init_ssao, destroy_ssao };
        this->ssao_buffer.bind();

        auto res_ssao = this->ssao_pass.init(
                "ssao_pass/vertex_shader.glsl",
                "ssao_pass/fragment_shader.glsl");

        if (res_ssao.isErr()) {
            LOG(util::log::LOG_LEVEL_ERROR, res_ssao.unwrapErr());
            std::exit(EXIT_FAILURE);
        }

        this->lighting_pass.use();
        this->lighting_pass.registerUniformLocation("g_position");
        this->lighting_pass.registerUniformLocation("g_normal");

        this->ssao_pass.registerUniformLocation("view");
        this->ssao_pass.registerUniformLocation("projection");

        this->ssao_buffer.unbind();
#endif
    }

    auto Renderer::prepare_frame(state::State &state) -> void {
        this->sub_renderer[RenderType::CHUNK_RENDERER]->_crtp_prepare_frame(state);
    }

    auto Renderer::frame(state::State &state) -> void {

        // geometry pass
        this->g_buffer.bind();
        this->sub_renderer[RenderType::CHUNK_RENDERER]->_crtp_frame(state);
        this->g_buffer.unbind();

#ifdef SSAO_PASS
        this->ssao_buffer.bind();
        glClear(GL_COLOR_BUFFER_BIT);

        this->ssao_pass.use();
        glBindVertexArray(this->quad_VAO);

        // position
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, this->g_buffer.buffer[0]);

        // normal
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, this->g_buffer.buffer[1]);

        auto view = state.player
                .get_camera()
                .get_view_matrix();

        auto projection = state.player
                .get_camera()
                .get_projection_matrix();

        // ssao shader runtime config
        this->ssao_pass["g_position"] = 0;
        this->ssao_pass["g_normal"] = 1;

        this->ssao_pass["view"] = view;
        this->ssao_pass["projection"] = projection;
        this->ssao_pass.upload_uniforms();

        glDrawArrays(GL_TRIANGLES, 0, 6);

        this->ssao_buffer.unbind();
#endif
        // atmosphere pass
        this->atmosphere_buffer.bind();
        glClear(GL_COLOR_BUFFER_BIT);

        this->skybox.frame(state);

        this->atmosphere_buffer.unbind();

        // base framebuffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // lighting pass
        this->lighting_pass.use();
        glBindVertexArray(this->quad_VAO);

        this->lighting_pass["g_position"] = 0;
        this->lighting_pass["g_normal"] = 1;
        this->lighting_pass["g_albedospec"] = 2;
        this->lighting_pass["g_atmosphere"] = 3;

#ifdef SSAO_PASS
        this->lighting_pass["g_ssao"] = 4;
#endif

        this->lighting_pass["light_direction"] = state.sun.get_orientation();
        this->lighting_pass["view_direction"] = state.player.get_camera().get_position();
        this->lighting_pass.upload_uniforms();

        // position
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, this->g_buffer.buffer[0]);

        // normal
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, this->g_buffer.buffer[1]);

        // color
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, this->g_buffer.buffer[2]);

        // atmosphere color
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, this->atmosphere_buffer.buffer[0]);

#ifdef SSAO_PASS
        // ssao color
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, this->ssao_buffer.buffer[0]);
#endif

        glDrawArrays(GL_TRIANGLES, 0, 6);

        // blitting the depth
        this->g_buffer.blit(0);

        // interface pass
        interface::set_camera_pos(state.player.get_camera().get_position());
        interface::update();
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

    auto Renderer::resize(i32 width, i32 height) -> void {
        this->g_buffer.resize(width, height);
        this->ssao_buffer.resize(width, height);

        // TODO: uncomment when implemented
        // this->ssao_blur_buffer.resize(width, height);

        this->atmosphere_buffer.resize(width, height);
    }
}
