//
// Created by Luis Ruisinger on 29.03.24.
//

#ifndef OPENGL_3D_ENGINE_ALIASES_H
#define OPENGL_3D_ENGINE_ALIASES_H

#include <cstdint>
#include <cmath>

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#define CHUNK_SIZE 64
#define RENDER_RADIUS 8
#define RENDER_DISTANCE (RENDER_RADIUS * CHUNK_SIZE)
#define Enum(_name, ...)                                                      \
    struct _name {                                                            \
        enum : i32 { __VA_ARGS__ };                                           \
        private:                                                              \
            struct enum_size { int __VA_ARGS__; };                            \
        public:                                                               \
            static constexpr  size_t count = sizeof(enum_size) / sizeof(i32); \
    }
#define FROM_INDEX(_i) (glm::vec2 {((_i) % RENDER_RADIUS) - RENDER_RADIUS, ((_i) / RENDER_RADIUS) - RENDER_RADIUS})

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using f32 = float_t;
using f64 = double_t;


#endif //OPENGL_3D_ENGINE_ALIASES_H
