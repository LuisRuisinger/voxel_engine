//
// Created by Luis Ruisinger on 23.08.24.
//

#ifndef OPENGL_3D_ENGINE_WINDOW_HANDLER_H
#define OPENGL_3D_ENGINE_WINDOW_HANDLER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "../../util/aliases.h"
#include "../threading/thread.h"
#include "../../util/log.h"
#include "../../util/result.h"

namespace core::io::window_handler {
    // TODO: later
    enum GLFWerror : u8 {
        TEST
    };

    class WindowHandler {
    public:
        WindowHandler();

        template<typename F, typename ...Args>
        requires std::invocable<F, Args...> &&
                 std::is_same_v<void, std::invoke_result_t<F &&, Args &&...>>
        auto add_framebuffer_size_callback(u64 id, F &&fun, Args &&...args) -> void {
            auto task = [
                    fun = std::forward<F>(fun),
                    ...args = std::forward<Args>(args)]() mutable -> decltype(auto) {
                WRAPPED_EXEC(std::invoke(fun, args...));
            };

            this->framebuffer_size_callbacks[id] = std::move(task);
        }

        template<typename F, typename ...Args>
        requires std::invocable<F, Args...> &&
                 std::is_same_v<void, std::invoke_result_t<F &&, Args &&...>>
        auto add_cursor_position_callback(u64 id, F &&fun, Args &&...args) -> void {
            auto task = [
                    fun = std::forward<F>(fun),
                    ...args = std::forward<Args>(args)]() mutable -> decltype(auto) {
                WRAPPED_EXEC(std::invoke(fun, args...));
            };

            this->cursor_position_callbacks[id] = std::move(task);
        }

        auto init() -> Result<GLFWwindow *, GLFWerror>;
        auto remove_framebuffer_size_callback(u64 id) -> void;
        auto remove_cursor_position_callback(u64 id) -> void;

        auto get_window() -> Result<GLFWwindow *, GLFWerror>;
        auto get_width() -> i32;
        auto get_height() -> i32;

        i32  width;
        i32  height;
        f32  xpos;
        f32  ypos;

    private:
        GLFWwindow *window;

        std::unordered_map<u64, std::function<void()>> framebuffer_size_callbacks;
        std::unordered_map<u64, std::function<void()>> cursor_position_callbacks;
    };
}


#endif //OPENGL_3D_ENGINE_WINDOW_HANDLER_H
