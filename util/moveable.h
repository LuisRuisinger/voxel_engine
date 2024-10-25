//
// Created by Luis Ruisinger on 25.04.24.
//

#ifndef OPENGL_3D_ENGINE_MOVEABLE_H
#define OPENGL_3D_ENGINE_MOVEABLE_H

#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <utility>

#include "defines.h"

namespace util {

    template<typename T>
    class Moveable {
    public:
        Moveable() = default;

        Moveable(const Moveable<T> &other) = delete;

        Moveable(Moveable<T> &&other) {
            *this = std::move(other);
        }

        auto operator=(const Moveable<T> &other) -> Moveable<T> & = delete;

        inline
        auto operator=(Moveable<T> &&other) noexcept -> Moveable<T> & {
            _t = other._t;
            other._t = T{};

            return *this;
        }

        inline
        auto operator=(const T &t) -> Moveable<T> & {
            _t = t;

            return *this;
        }

        inline operator T() {
            return _t;
        }


    private:
        T _t = T{};
    };
}

#endif //OPENGL_3D_ENGINE_MOVEABLE_H
