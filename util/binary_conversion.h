//
// Created by Luis Ruisinger on 30.06.24.
//

#ifndef OPENGL_3D_ENGINE_BINARY_CONVERSION_H
#define OPENGL_3D_ENGINE_BINARY_CONVERSION_H

#include "aliases.h"

#include <stdlib.h>
#include <type_traits>

namespace util::binary_conversion {
    template <typename T>
    requires std::is_integral<T>::value && (sizeof(T) <= 2)
    [[nodiscard]]
    INLINE static auto convert_to_binary(T t) -> u16 {
        return static_cast<u16>(t);
    }

    template <typename T>
    requires (std::is_floating_point<T>::value ||
             ((!std::is_integral<T>::value) && (!std::is_floating_point<T>::value))) &&
             (sizeof(T) <= 2)
    [[nodiscard]]
    INLINE static auto convert_to_binary(T t) -> u16 {
        u16 val;
        __builtin_memcpy(&val, &t, sizeof(T));

        return val;
    }

    template <typename T>
    requires std::is_integral<T>::value && (sizeof(T) <= 2)
    [[nodiscard]]
    INLINE static auto convert_from_binary(u16 val) -> T {
        return static_cast<T>(val);
    }

    template <typename T>
    requires std::is_floating_point<T>::value && (sizeof(T) <= 2)
    [[nodiscard]]
    INLINE static auto convert_from_binary(u16 val) -> T {
        T t;
        __builtin_memcpy(&t, &val, sizeof(T));

        return t;
    }
}

#endif //OPENGL_3D_ENGINE_BINARY_CONVERSION_H
