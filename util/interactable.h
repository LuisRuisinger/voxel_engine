//
// Created by Luis Ruisinger on 25.08.24.
//

#ifndef OPENGL_3D_ENGINE_INTERACTABLE_H
#define OPENGL_3D_ENGINE_INTERACTABLE_H

#include "../core/opengl/opengl_key_map.h"
#include "log.h"

namespace util::interactable {
    using namespace core::opengl::opengl_key_map;

    template <typename T>
    class Interactable {
    public:
        Interactable(OpenGLKeyMap &key_map)
            : key_map { key_map } {}

        template <Action action, Keymap key>
        auto on_input() -> void {
            DEBUG_LOG("add callback");
            this->key_map.add_callback(
                    action,
                    key,
                    [&]() -> void {
                        static_cast<T *>(this)->template on_input<action, key>();
                    });
        }

    private:
        OpenGLKeyMap &key_map;
    };
}

#define REGISTER_ACTION(_c, _a, _k, _f)   \
    template <>                           \
    auto _c::on_input<_a, _k>() -> void { \
        (_f);                             \
    }

#endif //OPENGL_3D_ENGINE_INTERACTABLE_H
