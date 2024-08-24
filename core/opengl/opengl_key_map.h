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
        ON_PRESSED = GLFW_PRESS,
        ON_RELEASE = GLFW_RELEASE,
        ON_REPEAT  = GLFW_REPEAT
    };

    class OpenGLKeyMap {
    public:
        OpenGLKeyMap();

        template<typename F>
        requires std::invocable<F, Keymap> &&
                 std::is_same_v<void, std::invoke_result_t<F &&, Keymap>>
        auto add_callback(Action action, Keymap key, F &&fun) -> void {
            auto task = [fun = std::forward<F>(fun)](Keymap arg) mutable -> decltype(auto) {
                WRAPPED_EXEC(std::invoke(fun, arg));
            };

            switch (action) {
                case ON_PRESSED: this->on_pressed[key] = std::move(std::invoke(fun, key)); break;
                case ON_RELEASE: this->on_release[key] = std::move(std::invoke(fun, key)); break;
                case ON_REPEAT : this->on_repeat[key]  = std::move(std::invoke(fun, key)); break;
            }
        }

        auto remove_callback(Action action, Keymap key) -> void;
        auto handle_event(std::pair<i32, i32> &ref) -> void;

    private:
        std::unordered_map<i32, Keymap> keys;

        std::unordered_map<Keymap, std::function<void(Keymap)>> on_pressed;
        std::unordered_map<Keymap, std::function<void(Keymap)>> on_release;
        std::unordered_map<Keymap, std::function<void(Keymap)>> on_repeat;
    };
}


#endif //OPENGL_3D_ENGINE_OPENGL_KEY_MAP_H
