//
// Created by Luis Ruisinger on 29.03.24.
//

#ifndef OPENGL_3D_ENGINE_ALIASES_H
#define OPENGL_3D_ENGINE_ALIASES_H

#include <cstdint>
#include <cmath>

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

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

#define CHUNK_SEGMENTS      16
#define MIN_HEIGHT          -128
#define HEIGHT_01           528
#define CHUNK_SIZE          32
#define RENDER_RADIUS       16
#define RENDER_DISTANCE     (RENDER_RADIUS * CHUNK_SIZE * 0.5F)
#define SQRT_2              1.4142135623730951F
#define DEFAULT_WIDTH       1920
#define DEFAULT_HEIGHT      1080
#define CACHE_LINE_SIZE     64
#define MAX_VERTICES_BUFFER (static_cast<u32>(131072 * 2.5))
#define LEFT_BIT            (static_cast<u64>(0x1)  << 55)
#define RIGHT_BIT           (static_cast<u64>(0x1)  << 54)
#define TOP_BIT             (static_cast<u64>(0x1)  << 53)
#define BOTTOM_BIT          (static_cast<u64>(0x1)  << 52)
#define FRONT_BIT           (static_cast<u64>(0x1)  << 51)
#define BACK_BIT            (static_cast<u64>(0x1)  << 50)
#define SET_FACES           (static_cast<u64>(0x3F) << 50)

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
