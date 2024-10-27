//
// Created by Luis Ruisinger on 15.03.24.
//

#include <algorithm>
#include <iostream>

#include "../core/rendering/renderer.h"
#include "../core/level/tiles/tile_manager.h"

#include "../util/sun.h"
#include "../util/indices_generator.h"
#include "../util/player.h"
#include "../util/color.h"

namespace core::rendering::renderer {
    auto Renderer::init_ImGui(GLFWwindow *window) -> void {
        interface::init(window);
    }

    auto Renderer::init_geometry_pass() -> void {
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
        get_sub_renderer(RenderType::CHUNK_RENDERER)._crtp_init();

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        glFrontFace(GL_CCW);

        auto res = this->g_pass.init(
                shader::Shader<shader::VERTEX_SHADER>("geometry_pass/vertex_shader.glsl"),
                shader::Shader<shader::FRAGMENT_SHADER>("geometry_pass/fragment_shader.glsl"));

        if (res.isErr()) {
            LOG(util::log::LOG_LEVEL_ERROR, res.unwrapErr());
            std::exit(EXIT_FAILURE);
        }

        // setting up uniforms
        this->g_pass.use();
        this->g_pass.register_uniform("view");
        this->g_pass.register_uniform("projection");
        this->g_pass.register_uniform("worldbase");
        this->g_pass.register_uniform("render_radius");
        this->g_pass.register_uniform("texture_array");

        this->g_buffer.unbind();
    }

    auto Renderer::init_shading_pass() -> void {
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        glEnable(GL_FRAMEBUFFER_SRGB);

        auto res = this->lighting_pass.init(
                shader::Shader<shader::VERTEX_SHADER>("shading_pass/vertex_shader.glsl"),
                shader::Shader<shader::FRAGMENT_SHADER>("shading_pass/fragment_shader.glsl"));

        if (res.isErr()) {
            LOG(util::log::LOG_LEVEL_ERROR, res.unwrapErr());
            std::exit(EXIT_FAILURE);
        }

        this->lighting_pass.use();
        this->lighting_pass.register_uniform("g_position");
        this->lighting_pass.register_uniform("g_normal");
        this->lighting_pass.register_uniform("g_albedospec");
        this->lighting_pass.register_uniform("g_depth");

        this->lighting_pass.register_uniform("g_atmosphere");
        this->lighting_pass.register_uniform("g_depth_map");
        this->lighting_pass.register_uniform("g_ssao");

        this->lighting_pass.register_uniform("g_water");
        this->lighting_pass.register_uniform("g_water_normal");
        this->lighting_pass.register_uniform("g_water_depth");

        // sun shading attributes
        this->lighting_pass.register_uniform("light_direction");
        this->lighting_pass.register_uniform("view_direction");

        this->lighting_pass.register_uniform("render_radius");

        // transformation matrices
        this->lighting_pass.register_uniform("view");
        this->lighting_pass.register_uniform("cascade_count");
        this->lighting_pass.register_uniform("far_z");

        for (auto i = 0; i < 4; ++i) {
            this->lighting_pass.register_uniform(
                    "cascade_plane_distances[" + std::to_string(i) + "]");
        }

        // ssr
        this->lighting_pass.register_uniform("g_albedospec_ssr");

        GLuint block_index1 = glGetUniformBlockIndex(this->lighting_pass.shader_id(), "LSMatrices");
        glUniformBlockBinding(this->lighting_pass.shader_id(), block_index1, 0);
    }

    auto Renderer::init_atmosphere_pass() -> void {
        auto init = [](framebuffer::Framebuffer &target, i32 width, i32 height) {

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

        auto destroy = [](framebuffer::Framebuffer &target) {
            glDeleteTextures(1, target.buffer.data());
        };

        this->atmosphere_buffer = { init, destroy };
        this->atmosphere_buffer.bind();

        this->skybox.init_shader();

        this->atmosphere_buffer.unbind();
    }

    auto Renderer::init_ssao_pass() -> void {
        auto init = [](framebuffer::Framebuffer &target, i32 width, i32 height) {

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

        auto destroy = [](framebuffer::Framebuffer &target) {
            glDeleteTextures(1, target.buffer.data());
        };

        this->ssao_buffer = { init, destroy };
        this->ssao_buffer.bind();

        auto res = this->ssao_pass.init(
                shader::Shader<shader::VERTEX_SHADER>("ssao_pass/vertex_shader.glsl"),
                shader::Shader<shader::FRAGMENT_SHADER>("ssao_pass/fragment_shader.glsl"));

        if (res.isErr()) {
            LOG(util::log::LOG_LEVEL_ERROR, res.unwrapErr());
            std::exit(EXIT_FAILURE);
        }

        this->ssao_pass.register_uniform("view");
        this->ssao_pass.register_uniform("projection");
        this->ssao_pass.register_uniform("g_normal");
        this->ssao_pass.register_uniform("g_depth");

        this->ssao_buffer.unbind();
    }

    auto Renderer::init_ssao_blur_pass() -> void {
        auto init = [](framebuffer::Framebuffer &target, i32 width, i32 height) {

            // allocate buffers
            target.buffer.resize(1);

            glGenTextures(1, &target.buffer[0]);
            glBindTexture(GL_TEXTURE_2D, target.buffer[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target.buffer[0], 0);
        };

        auto destroy = [](framebuffer::Framebuffer &target) {
            glDeleteTextures(1, target.buffer.data());
        };

        this->ssao_blur_buffer = { init, destroy };
        this->ssao_blur_buffer.bind();

        auto res = this->ssao_blur_pass.init(
                shader::Shader<shader::VERTEX_SHADER>("ssao_pass/vertex_shader.glsl"),
                shader::Shader<shader::FRAGMENT_SHADER>("ssao_blur_pass/fragment_shader.glsl"));

        if (res.isErr()) {
            LOG(util::log::LOG_LEVEL_ERROR, res.unwrapErr());
            std::exit(EXIT_FAILURE);
        }

        this->ssao_blur_pass.use();
        this->ssao_blur_pass.register_uniform("g_ssao");

        this->ssao_blur_buffer.unbind();
    }

    auto Renderer::init_ssr_pass() -> void {
        auto init = [](framebuffer::Framebuffer &target, i32 width, i32 height) {

            // allocate buffers
            target.buffer.resize(1);

            glGenTextures(1, &target.buffer[0]);
            glBindTexture(GL_TEXTURE_2D, target.buffer[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target.buffer[0], 0);
        };

        auto destroy = [](framebuffer::Framebuffer &target) {
            glDeleteTextures(1, target.buffer.data());
        };

        this->ssr_buffer = { init, destroy };
        this->ssr_buffer.bind();

        auto res = this->ssr_pass.init(
                shader::Shader<shader::VERTEX_SHADER>("ssao_pass/vertex_shader.glsl"),
                shader::Shader<shader::FRAGMENT_SHADER>("ssr_pass/fragment_shader.glsl"));

        if (res.isErr()) {
            LOG(util::log::LOG_LEVEL_ERROR, res.unwrapErr());
            std::exit(EXIT_FAILURE);
        }

        this->ssr_pass.use();
        this->ssr_pass.register_uniform("g_atmosphere");
        this->ssr_pass.register_uniform("g_albedospec");
        this->ssr_pass.register_uniform("g_depth");
        this->ssr_pass.register_uniform("g_water_depth");
        this->ssr_pass.register_uniform("g_water_normal");

        this->ssr_pass.register_uniform("view");
        this->ssr_pass.register_uniform("projection");

        this->ssr_buffer.unbind();
    }

    auto Renderer::init_ssr_blur_pass() -> void {
        auto init = [](framebuffer::Framebuffer &target, i32 width, i32 height) {

            // allocate buffers
            target.buffer.resize(1);

            glGenTextures(1, &target.buffer[0]);
            glBindTexture(GL_TEXTURE_2D, target.buffer[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target.buffer[0], 0);
        };

        auto destroy = [](framebuffer::Framebuffer &target) {
            glDeleteTextures(1, target.buffer.data());
        };

        this->ssr_blur_buffer = { init, destroy };
        this->ssr_blur_buffer.bind();

        auto res = this->ssr_blur_pass.init(
                shader::Shader<shader::VERTEX_SHADER>("ssao_pass/vertex_shader.glsl"),
                shader::Shader<shader::FRAGMENT_SHADER>("ssr_blur_pass/fragment_shader.glsl"));

        if (res.isErr()) {
            LOG(util::log::LOG_LEVEL_ERROR, res.unwrapErr());
            std::exit(EXIT_FAILURE);
        }

        this->ssr_blur_pass.use();
        this->ssr_blur_pass.register_uniform("g_ssr");

        this->ssr_blur_buffer.unbind();
    }

    auto Renderer::init_depth_map_pass() -> void {
        auto init = [](framebuffer::Framebuffer &target, i32 width, i32 height) {

            // allocate buffer
            target.buffer.resize(1);
            const u32 shadow_resolution = RENDER_RADIUS * 2 * CHUNK_SIZE;

            glGenTextures(1, &target.buffer[0]);
            glBindTexture(GL_TEXTURE_2D_ARRAY, target.buffer[0]);
            glTexImage3D(
                    GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F,
                    shadow_resolution, shadow_resolution, 4,
                    0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

            const f32 border_color[] = { 1.0F, 1.0F, 1.0F, 1.0F };
            glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, border_color);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target.buffer[0], 0);

            // explicitly telling OpenGL that we won't read or write color data
            // a framebuffer cannot be complete without a single color buffer
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
        };

        auto destroy = [](framebuffer::Framebuffer &target) {
            glDeleteTextures(1, target.buffer.data());
        };

        this->depth_map_buffer = { init, destroy };
        this->depth_map_buffer.bind();

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0F, 1.0F);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        glFrontFace(GL_CCW);

        // lighting pass
        auto res = this->depth_map_pass.init(
                shader::Shader<shader::VERTEX_SHADER>("depth_map_pass/vertex_shader.glsl"),
                shader::Shader<shader::GEOMETRY_SHADER>("depth_map_pass/geometry_shader.glsl"),
                shader::Shader<shader::FRAGMENT_SHADER>("depth_map_pass/fragment_shader.glsl"));

        if (res.isErr()) {
            LOG(util::log::LOG_LEVEL_ERROR, res.unwrapErr());
            std::exit(EXIT_FAILURE);
        }

        this->depth_map_pass.use();
        this->depth_map_pass.register_uniform("worldbase");
        this->depth_map_pass.register_uniform("render_radius");

        GLuint block_index = glGetUniformBlockIndex(this->depth_map_pass.shader_id(), "LSMatrices");
        glUniformBlockBinding(this->depth_map_pass.shader_id(), block_index, 0);

        glGenBuffers(1, &this->ls_matrices_UBO);
        glBindBuffer(GL_UNIFORM_BUFFER, this->ls_matrices_UBO);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4x4) * 4, nullptr, GL_STATIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, this->ls_matrices_UBO);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        this->depth_map_buffer.unbind();
    }

    auto Renderer::init_water_pass() -> void {
        auto init = [](framebuffer::Framebuffer &target, i32 width, i32 height) {

            // allocate buffers
            target.buffer.resize(3);

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
            u32 attachments[2] = {
                    GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1
            };

            glDrawBuffers(2, attachments);
        };

        auto destroy = [](framebuffer::Framebuffer &target) {
            glDeleteTextures(4, target.buffer.data());
            glDeleteRenderbuffers(1, &target.buffer[3]);
        };

        this->water_buffer = { init, destroy };
        this->water_buffer.bind();
        get_sub_renderer(RenderType::WATER_RENDERER)._crtp_init();

        OPENGL_VERIFY(glGenTextures(1, &this->water_normal_tex));
        OPENGL_VERIFY(glBindTexture(GL_TEXTURE_2D, this->water_normal_tex));

        // Set texture parameters
        OPENGL_VERIFY(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        OPENGL_VERIFY(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        OPENGL_VERIFY(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
        OPENGL_VERIFY(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));

        i32 width, height, nr_channels;
        auto data = stbi_load("../resources/maps/water_normal.jpg", &width, &height, &nr_channels, 0);

        OPENGL_VERIFY(
                glTexImage2D(
                        GL_TEXTURE_2D, 0, GL_RGB,
                        width, height, 0,
                        GL_RGB, GL_UNSIGNED_BYTE, data));

        OPENGL_VERIFY(glBindTexture(GL_TEXTURE_2D, 0));

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        auto res = this->water_pass.init(
                shader::Shader<shader::VERTEX_SHADER>("water_pass/vertex_shader.glsl"),
                shader::Shader<shader::FRAGMENT_SHADER>("water_pass/fragment_shader.glsl"));

        if (res.isErr()) {
            LOG(util::log::LOG_LEVEL_ERROR, res.unwrapErr());
            std::exit(EXIT_FAILURE);
        }

        // setting up uniforms
        this->water_pass.use();
        this->water_pass.register_uniform("view");
        this->water_pass.register_uniform("projection");
        this->water_pass.register_uniform("worldbase");
        this->water_pass.register_uniform("render_radius");
        this->water_pass.register_uniform("water_normal_tex");

        this->water_buffer.unbind();
    }

    auto Renderer::init_pipeline() -> void {

        // screen-space quad
        // used for screen space passes
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
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                              4 * sizeof(float), nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                              4 * sizeof(float), reinterpret_cast<void *>(2 * sizeof(float)));

        init_geometry_pass();
        init_shading_pass();
        init_atmosphere_pass();
        init_ssr_pass();
        init_ssr_blur_pass();
        init_ssao_pass();
        init_ssao_blur_pass();
        init_depth_map_pass();
        init_water_pass();
    }

    auto Renderer::prepare_frame(state::State &state) -> void {
        for (auto &[_, v] : this->sub_renderer)
            v->prepare_frame(state);
    }

    auto Renderer::frame(state::State &state) -> void {
        auto player_pos = state.player.get_camera().get_position();
        auto player_view = state.player.get_camera().get_view_matrix();
        auto player_projection = state.player.get_camera().get_projection_matrix();

        const auto sun_orientation = state.sun.get_orientation();
        const auto world_pos = state.platform.get_world_root();
        const auto &ls_matrices = state.sun.light_space_matrices;

        glBindBuffer(GL_UNIFORM_BUFFER, this->ls_matrices_UBO);

        for (auto i = 0; i < ls_matrices.size(); ++i) {
            glBufferSubData(
                    GL_UNIFORM_BUFFER,
                    i * sizeof(glm::mat4x4),
                    sizeof(glm::mat4x4),
                    &ls_matrices[i]);
        }

        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        glViewport(0, 0, RENDER_RADIUS * 2 * CHUNK_SIZE, RENDER_RADIUS * 2 * CHUNK_SIZE);

        this->depth_map_buffer.bind();
        glClear(GL_DEPTH_BUFFER_BIT);

        this->depth_map_pass.use();
        this->depth_map_pass["worldbase"] = world_pos;
        this->depth_map_pass["render_radius"] = static_cast<u32>(RENDER_RADIUS);
        this->depth_map_pass.upload_uniforms();

        get_sub_renderer(RenderType::CHUNK_RENDERER)._crtp_frame(state);
        this->depth_map_buffer.unbind();

        glViewport(0, 0, this->g_buffer.get_width(), this->g_buffer.get_height());

        // geometry pass
        this->g_buffer.bind();

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // binding the 2D texture array containing the textures of all tiles
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, level::tiles::tile_manager::tile_manager.texture_array);

        this->g_pass.use();
        this->g_pass["view"] = player_view;
        this->g_pass["projection"] = player_projection;
        this->g_pass["worldbase"] = world_pos;
        this->g_pass["render_radius"] = static_cast<u32>(RENDER_RADIUS);
        this->g_pass["texture_array"] = 0;
        this->g_pass.upload_uniforms();

        get_sub_renderer(RenderType::CHUNK_RENDERER)._crtp_frame(state);
        this->g_buffer.unbind();

        /*
         *
         *
         *
         * water pass
         *
         *
         *
         */

        this->water_buffer.bind();

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, this->water_normal_tex);

        this->water_pass.use();
        this->water_pass["view"] = player_view;
        this->water_pass["projection"] = player_projection;
        this->water_pass["worldbase"] = world_pos;
        this->water_pass["render_radius"] = static_cast<u32>(RENDER_RADIUS);
        this->water_pass["water_normal_tex"] = 0;
        this->water_pass.upload_uniforms();

        get_sub_renderer(RenderType::WATER_RENDERER)._crtp_frame(state);
        this->water_buffer.unbind();

        /*
         *
         *
         * SSR
         *
         *
         */

        this->ssr_buffer.bind();
        glClear(GL_COLOR_BUFFER_BIT);

        this->ssr_pass.use();
        glBindVertexArray(this->quad_VAO);

        this->ssr_pass["g_albedospec"] = 0;
        this->ssr_pass["g_depth"] = 1;
        this->ssr_pass["g_atmosphere"] = 2;
        this->ssr_pass["g_water_normal"] = 3;
        this->ssr_pass["g_water_depth"] = 4;

        this->ssr_pass["view"] = player_view;
        this->ssr_pass["projection"] = player_projection;
        this->ssr_pass.upload_uniforms();

        // color
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, this->g_buffer.buffer[2]);

        // g buffer fragment depth
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, this->g_buffer.buffer[3]);

        // atmosphere color
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, this->atmosphere_buffer.buffer[0]);

        // water normal
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, this->water_buffer.buffer[1]);

        // water fragment depth
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, this->water_buffer.buffer[3]);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        this->ssr_buffer.unbind();

        /*
        *
        *
        * SSR blur
        *
        *
        */
        /*
        this->ssr_blur_buffer.bind();
        glClear(GL_COLOR_BUFFER_BIT);

        this->ssr_blur_pass.use();
        glBindVertexArray(this->quad_VAO);

        // ssao pass unfiltered
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, this->ssr_buffer.buffer[0]);;

        this->ssr_blur_pass["g_ssr"] = 0;
        this->ssr_blur_pass.upload_uniforms();

        glDrawArrays(GL_TRIANGLES, 0, 6);
        this->ssr_blur_buffer.unbind();
         */

        /*
         *
         *
         * SSAO
         *
         *
         */
        this->ssao_buffer.bind();
        glClear(GL_COLOR_BUFFER_BIT);

        this->ssao_pass.use();
        glBindVertexArray(this->quad_VAO);

        // geometry pass normal
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, this->g_buffer.buffer[1]);

        // geometry pass depth
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, this->g_buffer.buffer[3]);

        // ssao shader runtime config
        this->ssao_pass["g_normal"] = 0;
        this->ssao_pass["g_depth"] = 1;

        this->ssao_pass["view"] = player_view;
        this->ssao_pass["projection"] = player_projection;
        this->ssao_pass.upload_uniforms();

        glDrawArrays(GL_TRIANGLES, 0, 6);

        this->ssao_buffer.unbind();

        // atmosphere pass
        this->atmosphere_buffer.bind();
        glClear(GL_COLOR_BUFFER_BIT);
        this->skybox.frame(state, player_view, player_projection);;
        this->atmosphere_buffer.unbind();

        /*
        *
        *
        * SSAO blur
        *
        *
        */
        this->ssao_blur_buffer.bind();
        glClear(GL_COLOR_BUFFER_BIT);

        this->ssao_blur_pass.use();
        glBindVertexArray(this->quad_VAO);

        // ssao pass unfiltered
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, this->ssao_buffer.buffer[0]);;

        this->ssao_blur_pass["g_ssao"] = 0;
        this->ssao_blur_pass.upload_uniforms();

        glDrawArrays(GL_TRIANGLES, 0, 6);
        this->ssao_blur_buffer.unbind();

        /*
         *
         *
         * lighting + scene combination
         *
         *
         */

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        this->lighting_pass.use();
        glBindVertexArray(this->quad_VAO);

        this->lighting_pass["g_position"] = 0;
        this->lighting_pass["g_normal"] = 1;
        this->lighting_pass["g_albedospec"] = 2;
        this->lighting_pass["g_depth"] = 3;

        this->lighting_pass["g_atmosphere"] = 4;
        this->lighting_pass["g_depth_map"] = 5;
        this->lighting_pass["g_ssao"] = 6;

        this->lighting_pass["g_water"] = 7;
        this->lighting_pass["g_water_normal"] = 8;
        this->lighting_pass["g_water_depth"] = 9;

        this->lighting_pass["light_direction"] = sun_orientation;
        this->lighting_pass["view_direction"] = player_pos;

        this->lighting_pass["render_radius"] = static_cast<u32>(RENDER_RADIUS);

        this->lighting_pass["view"] = player_view;
        this->lighting_pass["far_z"] = 640.0F;
        this->lighting_pass["cascade_count"] = static_cast<i32>(ls_matrices.size());

        for (auto i = 0; i < state.sun.shadow_cascades_level.size(); ++i)
            this->lighting_pass["cascade_plane_distances[" + std::to_string(i) + "]"] =
                    state.sun.shadow_cascades_level[i];

        this->lighting_pass["g_albedospec_ssr"] = 10;

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

        // g buffer fragment depth
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, this->g_buffer.buffer[3]);

        // atmosphere color
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, this->atmosphere_buffer.buffer[0]);

        // shadow map depth
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D_ARRAY, this->depth_map_buffer.buffer[0]);

        // ssao color
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, this->ssao_blur_buffer.buffer[0]);

        // water position
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, this->water_buffer.buffer[0]);

        // water normal
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D, this->water_buffer.buffer[1]);

        // water fragment depth
        glActiveTexture(GL_TEXTURE9);
        glBindTexture(GL_TEXTURE_2D, this->water_buffer.buffer[3]);

        // ssr color
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_2D, this->ssr_buffer.buffer[0]);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        // interface pass
        interface::set_camera_pos(player_pos);
        interface::update();
        interface::render();
    }

    auto Renderer::get_sub_renderer(RenderType render_type) -> Renderable<BaseInterface> & {
        for (const auto &[k, v] : this->sub_renderer)
            if (k == render_type)
                return *v;

#if defined(__GNUC__)
        __builtin_unreachable();
#elif defined(_MSC_VER)
        __assume(false);
#endif
    }

    auto Renderer::add_sub_renderer(RenderType type, Renderable<BaseInterface> *renderer) -> void {
        this->sub_renderer.emplace_back(type, renderer);
    }

    auto Renderer::remove_sub_renderer(RenderType render_type) -> void {
        this->sub_renderer.erase(
                std::remove_if(
                        this->sub_renderer.begin(),
                        this->sub_renderer.end(),
                        [render_type](auto &e) {
                            return e.first == render_type;
                        }));
    }

    auto Renderer::resize(i32 width, i32 height) -> void {
        LOG(width, height);

        this->g_buffer.resize(width, height);
        this->ssao_buffer.resize(width, height);
        this->ssao_blur_buffer.resize(width, height);
        this->atmosphere_buffer.resize(width, height);
        this->water_buffer.resize(width, height);
    }
}
