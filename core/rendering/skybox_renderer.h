//
// Created by Luis Ruisinger on 13.09.24.
//

#ifndef OPENGL_3D_ENGINE_SKYBOX_RENDERER_H
#define OPENGL_3D_ENGINE_SKYBOX_RENDERER_H

#include "../util/aliases.h"
#include "../util/renderable.h"

namespace core::rendering::skybox_renderer {
    class SkyboxRenderer {
    public:
        SkyboxRenderer() =default;

        // renderable
        auto init_shader() -> void;
        auto prepare_frame(state::State &) -> void;
        auto frame(state::State &) -> void;

    private:
        GLuint VAO;
        GLuint VBO;
        GLuint EBO;

        shader::Shader shader;

        u32 mie_tex;
        u32 ray_tex;

        u32 indices_amount;

        GLuint ambient_texture;
        GLuint sunlight_texture;
    };
}

#endif //OPENGL_3D_ENGINE_SKYBOX_RENDERER_H
