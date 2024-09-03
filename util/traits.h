//
// Created by Luis Ruisinger on 01.09.24.
//

#ifndef OPENGL_3D_ENGINE_TRAITS_H
#define OPENGL_3D_ENGINE_TRAITS_H

namespace core::state {
    struct State;
}

namespace util::traits {
    template <typename T>
    class Updateable {
    public:
        auto update(core::state::State &state) -> void {
            static_cast<T *>(this)->update(state);
        }
    };

    template <typename T>
    class Tickable {
    public:
        auto tick(core::state::State &state) -> void {
            static_cast<T *>(this)->tick(state);
        }
    };
}

#endif //OPENGL_3D_ENGINE_TRAITS_H
