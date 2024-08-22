//
// Created by Luis Ruisinger on 29.03.24.
//

#ifndef OPENGL_3D_ENGINE_ALIASES_H
#define OPENGL_3D_ENGINE_ALIASES_H

#include <cstdint>
#include <cmath>

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#define CHUNK_SEGMENTS  16
#define MIN_HEIGHT      -128
#define CHUNK_SIZE      32
#define RENDER_RADIUS   12
#define RENDER_DISTANCE (RENDER_RADIUS * CHUNK_SIZE * 0.5F)

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

#define Enum(_name, ...)                                                     \
    struct _name {                                                           \
        enum : i32 { __VA_ARGS__ };                                          \
        private:                                                             \
            struct enum_size { i32 __VA_ARGS__; };                           \
        public:                                                              \
            static constexpr size_t count = sizeof(enum_size) / sizeof(i32); \
    }

#if defined(__GNUC__) || defined(__clang__)
    #define INLINE __attribute__((always_inline))
#elif defined(_MSC_VER)
    #define INLINE __forceinline
#else
    #define INLINE inline
#endif

#ifdef __AVX2__
#include <immintrin.h>
    #define VERTEX __m256i
#else
    #define VERTEX u64
#endif

#endif //OPENGL_3D_ENGINE_ALIASES_H
