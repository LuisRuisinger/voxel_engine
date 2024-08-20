//
// Created by Luis Ruisinger on 14.06.24.
//

#ifndef OPENGL_3D_ENGINE_ASSERT_H
#define OPENGL_3D_ENGINE_ASSERT_H

#include "log.h"

#include <type_traits>
#include <string>
#include <cstdlib>
#include <iostream>
#include <csignal>

#if defined(_MSC_VER)
    #define DEBUG_BREAK() __debugbreak()
#elif defined(__unix__) || defined(__APPLE__)
    #define DEBUG_BREAK() std::raise(SIGTRAP)
#else
    #define DEBUG_BREAK() std::abort()
#endif

#ifdef DEBUG
    #define ASSERT_IMPL(_x, _m) ({                                         \
        static_assert(std::is_convertible_v<decltype((_x)), bool>);        \
        static_assert(std::is_convertible_v<decltype((_m)), std::string>); \
        if (!(_x)) {                                                       \
            DEBUG_LOG(                                                     \
                util::log::Level::LOG_LEVEL_ERROR,                         \
                "ASSERTION FAILED" + (std::string(_m).empty()              \
                    ? (": " + std::string(_m))                             \
                    : ""));                                                \
            DEBUG_BREAK();                                                 \
        }})
#else
    #define ASSERT_IMPL(_x, _m)
#endif

#define ASSERT_EQ_1(_x) \
    ASSERT_IMPL(_x, "")

#define ASSERT_EQ_2(_x, _m) \
    ASSERT_IMPL(_x, _m)

#define ASSERT_NEQ_1(_x) \
    ASSERT_IMPL(!(_x), "")

#define ASSERT_NEQ_2(_x, _m) \
    ASSERT_IMPL(!(_x), _m)

#define GET_MACRO(_1, _2, NAME, ...) NAME

#define ASSERT_EQ(...) \
    GET_MACRO(__VA_ARGS__, ASSERT_EQ_2, ASSERT_EQ_1)(__VA_ARGS__)

#define ASSERT_NEQ(...) \
    GET_MACRO(__VA_ARGS__, ASSERT_NEQ_2, ASSERT_NEQ_1)(__VA_ARGS__)

#endif //OPENGL_3D_ENGINE_ASSERT_H
