//
// Created by Luis Ruisinger on 24.08.24.
//

#ifndef OPENGL_3D_ENGINE_OPENGL_KEY_MAP_H
#define OPENGL_3D_ENGINE_OPENGL_KEY_MAP_H

#include <memory>
#include <map>

#include "../../util/aliases.h"
#include "../../util/keymap.h"

namespace core::opengl::opengl_key_map {
    enum Action : u32 {
        ON_RELEASE = 0,
        ON_PRESSED = 1,
        ON_REPEAT  = 2
    };

    class OpenGLKeyMap {
    public:
        OpenGLKeyMap();

        template<typename F, typename ...Args>
        requires std::invocable<F, Args...> &&
                 std::is_same_v<void, std::invoke_result_t<F &&, Args &&...>>
        auto add_callback(Action action, Keymap key, F &&fun, Args &&...args) -> void {
            auto task = [
                    fun = std::forward<F>(fun),
                    ...args = std::forward<Args>(args)]() mutable -> decltype(auto) {
                std::invoke(fun, args...);
            };

            switch (action) {
                case ON_RELEASE: this->on_release[key] = std::move(task); break;

                // enqueues a function that should be called repeatedly also on
                // pressed to capture the first occurence of the key callback
                case ON_REPEAT : this->on_repeat[key]  = std::move(task);
                case ON_PRESSED: this->on_pressed[key] = std::move(task); break;
            }
        }

        auto remove_callback(Action action, Keymap key) -> void;
        auto handle_event(std::pair<i32, i32> &ref) -> void;
        auto run_repeat() -> void;

    private:
        std::unordered_map<i32, Keymap> keys;

        std::unordered_map<Keymap, std::function<void()>> on_pressed;
        std::unordered_map<Keymap, std::function<void()>> on_release;
        std::unordered_map<Keymap, std::function<void()>> on_repeat;

        std::map<Keymap, std::function<void()>> repeat_functions;
    };
}


#endif //OPENGL_3D_ENGINE_OPENGL_KEY_MAP_H
