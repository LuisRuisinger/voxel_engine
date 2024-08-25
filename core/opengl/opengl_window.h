//
// Created by Luis Ruisinger on 23.08.24.
//

#ifndef OPENGL_3D_ENGINE_OPENGL_WINDOW_H
#define OPENGL_3D_ENGINE_OPENGL_WINDOW_H

#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "../../util/aliases.h"
#include "../../util/log.h"
#include "../../util/result.h"

namespace core::opengl::opengl_window {

    enum Vsync : u8 {
        ENABLED  = 1,
        DISABLED = 0
    };

    class OpenGLWindow {
        using SizeToken = std::pair<i32, i32>;
        using PosToken  = std::pair<f32, f32>;
        using KeyToken  = std::pair<i32, i32>;

    public:
        OpenGLWindow();

        template<typename F>
        requires std::invocable<F, SizeToken &> &&
                 std::is_same_v<void, std::invoke_result_t<F &&, SizeToken &>>
        auto add_framebuffer_size_callback(u64 id, F &&fun) -> void {
            this->framebuffer_size_callbacks[id] = std::move(fun);
        }

        template<typename F>
        requires std::invocable<F, PosToken &> &&
                 std::is_same_v<void, std::invoke_result_t<F &&, PosToken &>>
        auto add_cursor_position_callback(u64 id, F &&fun) -> void {
            this->cursor_position_callbacks[id] = std::move(fun);
        }

        template<typename F>
        requires std::invocable<F, KeyToken &> &&
                 std::is_same_v<void, std::invoke_result_t<F &&, KeyToken &>>
        auto add_key_callback(u64 id, F &&fun) -> void {
            this->key_callbacks[id] = std::move(fun);
        }

        auto init() -> GLFWwindow *;
        auto set_vsync(Vsync) -> void;
        auto get_window() -> GLFWwindow *;

        auto remove_framebuffer_size_callback(u64 id) -> void;
        auto remove_cursor_position_callback(u64 id) -> void;
        auto remove_key_callback(u64 id) -> void;

    private:
        GLFWwindow *window;

        std::unordered_map<u64, std::function<void(SizeToken &)>> framebuffer_size_callbacks;
        std::unordered_map<u64, std::function<void(PosToken  &)>> cursor_position_callbacks;
        std::unordered_map<u64, std::function<void(KeyToken  &)>> key_callbacks;
    };
}


#endif //OPENGL_3D_ENGINE_OPENGL_WINDOW_H
