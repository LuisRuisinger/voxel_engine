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

#define SSAO_PASS

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

        this->atmosphere_buffer.unbind();

        // depth map pass
        auto init_depth_map_pass = [](framebuffer::Framebuffer &target, i32 width, i32 height) {

            // allocate buffer
            target.buffer.resize(1);
            const u32 shadow_width_height = RENDER_RADIUS * 2 * CHUNK_SIZE;

            glGenTextures(1, &target.buffer[0]);
            glBindTexture(GL_TEXTURE_2D, target.buffer[0]);
            glTexImage2D(
                    GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F,
                    shadow_width_height, shadow_width_height, 0,
                    GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

            f32 border_color[] = {
                    1.0F, 1.0F, 1.0F, 1.0F
            };

            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, target.buffer[0], 0);

            // explicitly telling OpenGL that we won't read or write color data
            // a framebuffer cannot be complete without a single color buffer
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
        };

        auto destroy_depth_map_pass = [](framebuffer::Framebuffer &target) {
            glDeleteTextures(1, target.buffer.data());
        };

        this->depth_map_buffer = { init_depth_map_pass, destroy_depth_map_pass };
        this->depth_map_buffer.bind();
        glEnable(GL_DEPTH_TEST);
        glCullFace(GL_FRONT);

        // lighting pass
        auto res_depth_map_pass = this->depth_map_pass.init(
                "depth_map_pass/vertex_shader.glsl",
                "depth_map_pass/fragment_shader.glsl");

        if (res_depth_map_pass.isErr()) {
            LOG(util::log::LOG_LEVEL_ERROR, res_depth_map_pass.unwrapErr());
            std::exit(EXIT_FAILURE);
        }

        this->depth_map_pass.use();
        this->depth_map_pass.registerUniformLocation("view");
        this->depth_map_pass.registerUniformLocation("projection");
        this->depth_map_pass.registerUniformLocation("worldbase");
        this->depth_map_pass.registerUniformLocation("render_radius");

        this->depth_map_buffer.unbind();

        /*
         *
         *
         *
         * done in the default frame buffer
         *
         *
         *
         */

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
        this->lighting_pass.registerUniformLocation("g_depth_map");

#ifdef SSAO_PASS
        this->lighting_pass.registerUniformLocation("g_ssao");
#endif

        // sun shading attributes
        this->lighting_pass.registerUniformLocation("light_direction");
        this->lighting_pass.registerUniformLocation("view_direction");

        // transformation matrices
        this->lighting_pass.registerUniformLocation("depth_map_view");
        this->lighting_pass.registerUniformLocation("depth_map_projection");

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

            // fragment depth buffer
            glGenTextures(1, &target.buffer[3]);
            glBindTexture(GL_TEXTURE_2D, target.buffer[3]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, target.buffer[3], 0);

            // opengl needs to know which color attachments to use for the rendering of
            // this framebuffer
            u32 attachments[3] = {
                    GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2
            };

            glDrawBuffers(3, attachments);
        };

        auto destroy = [](framebuffer::Framebuffer &target) {
            glDeleteTextures(4, target.buffer.data());
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
        auto player_pos = state.player.get_camera().get_position();
        auto player_view = state.player.get_camera().get_view_matrix();
        auto player_projection = state.player.get_camera().get_projection_matrix();

        auto sun_orientation = state.sun.get_orientation();
        auto sun_view = state.sun.get_view_matrix();
        auto sun_projection = state.sun.get_projection_matrix();


        // depth map pass
        // injecting another shader into the chunk renderer to just extract depth information
        // TODO: change this later
        glViewport(0, 0, RENDER_RADIUS * 2 * CHUNK_SIZE, RENDER_RADIUS * 2 * CHUNK_SIZE);
        this->depth_map_buffer.bind();
        glClear(GL_DEPTH_BUFFER_BIT);

        this->depth_map_pass.use();
        this->depth_map_pass["view"] = sun_view;
        this->depth_map_pass["projection"] = sun_projection;

        this->depth_map_pass["worldbase"] = state.platform.get_world_root();
        this->depth_map_pass["render_radius"] = static_cast<u32>(RENDER_RADIUS);
        this->depth_map_pass.upload_uniforms();

        this->sub_renderer[RenderType::CHUNK_RENDERER]->_crtp_frame_inject_shader(
                state,
                sun_view,
                sun_projection);
        this->depth_map_buffer.unbind();

        // geometry pass
        glViewport(0, 0, this->g_buffer.get_width(), this->g_buffer.get_height());
        this->g_buffer.bind();
        this->sub_renderer[RenderType::CHUNK_RENDERER]->_crtp_frame(
                state,
                player_view,
                player_projection);
        this->g_buffer.unbind();

#ifdef SSAO_PASS
        this->ssao_buffer.bind();
        glClear(GL_COLOR_BUFFER_BIT);

        this->ssao_pass.use();
        glBindVertexArray(this->quad_VAO);

        // geometry pass fragment depth
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, this->g_buffer.buffer[3]);

        // ssao shader runtime config
        this->ssao_pass["g_depth"] = 0;

        this->ssao_pass["view"] = player_view;
        this->ssao_pass["projection"] = player_projection;
        this->ssao_pass.upload_uniforms();

        glDrawArrays(GL_TRIANGLES, 0, 6);

        this->ssao_buffer.unbind();
#endif
        // atmosphere pass
        this->atmosphere_buffer.bind();
        glClear(GL_COLOR_BUFFER_BIT);
        this->skybox.frame(state, player_view, player_projection);;
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
        this->lighting_pass["g_depth_map"] = 4;

#ifdef SSAO_PASS
        this->lighting_pass["g_ssao"] = 5;
#endif

        this->lighting_pass["light_direction"] = sun_orientation;
        this->lighting_pass["view_direction"] = player_pos;

        this->lighting_pass["depth_map_view"] = sun_view;
        this->lighting_pass["depth_map_projection"] = sun_projection;
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

        // shadow map depth
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, this->depth_map_buffer.buffer[0]);

#ifdef SSAO_PASS
        // ssao color
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, this->ssao_buffer.buffer[0]);
#endif

        glDrawArrays(GL_TRIANGLES, 0, 6);

        // interface pass
        interface::set_camera_pos(player_pos);
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
