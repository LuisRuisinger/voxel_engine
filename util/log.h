//
// Created by Luis Ruisinger on 13.06.24.
//

#ifndef OPENGL_3D_ENGINE_LOG_H
#define OPENGL_3D_ENGINE_LOG_H

#include <iostream>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include "defines.h"
#include "reflections.h"

namespace util::log {
    enum Level : u8 {
        LOG_LEVEL_NORMAL,
        LOG_LEVEL_WARN,
        LOG_LEVEL_ERROR,
        LOG_LEVEL_DEBUG
    };

    template <typename T>
    struct is_address {
        static const bool value = false;
    };

    template <typename T>
    requires (std::is_pointer_v<T> &&
              !std::is_same_v<T, char *> &&
              !std::is_same_v<T, const char *>)
    struct is_address<T> {
        static const bool value = true;
    };

    template <typename T>
    struct is_vec {
        static const bool value = false;
    };

    template <glm::length_t L, typename T, glm::qualifier Q>
    struct is_vec<glm::vec<L, T, Q>> {
        static const bool value = true;
    };

    template <typename T>
    struct is_mat {
        static const bool value = false;
    };

    template <glm::length_t C, glm::length_t R, class T, glm::qualifier Q>
    struct is_mat<glm::mat<C, R, T, Q>> {
        static const bool value = true;
    };

    auto print(
            std::string_view out,
            Level kind = Level::LOG_LEVEL_DEBUG,
            std::string file = __builtin_FILE(),
            std::string caller = __builtin_FUNCTION(),
            const size_t line = __builtin_LINE()) -> void;

    template<typename T>
    requires (!std::is_convertible<T, std::string>::value &&
              !std::is_same<T, std::stringstream>::value)
    auto print(
            const T &out,
            Level kind = Level::LOG_LEVEL_DEBUG,
            std::string file = __builtin_FILE(),
            std::string caller = __builtin_FUNCTION(),
            const size_t line = __builtin_LINE()) -> decltype(std::to_string(out), void()) {
        log::print(std::string_view(out), kind, file, caller, line);
    }

    inline auto print(
            std::stringstream &out,
            Level kind = Level::LOG_LEVEL_DEBUG,
            std::string file = __builtin_FILE(),
            std::string caller = __builtin_FUNCTION(),
            const size_t line = __builtin_LINE()) -> void {
        log::print(out.str(), kind, file, caller, line);
    }

    struct _End {};
    struct _Log {
        _Log(std::string file, std::string caller, const size_t line)
            : file   { file   },
              caller { caller },
              line   { line   }
        {}

        template <typename T>
        requires (is_mat<T>::value || is_vec<T>::value)
        auto operator<<(const T &t) -> _Log & {
            this->s_stream << glm::to_string(t) << " ";
            return *this;
        }

        template <typename T>
        requires is_address<T>::value
        auto operator<<(const T &t) -> _Log & {
            this->s_stream << "0x" << std::hex << reinterpret_cast<u64>(t) << std::dec << " ";
            return *this;
        }

        template <typename T>
        requires std::is_integral_v<T>
        auto operator<<(const T &t) -> _Log & {
            this->s_stream << std::to_string(t) << " ";
            return *this;
        }

        template <typename T>
        requires std::is_same_v<T, _End>
        auto operator<<(const T &) -> void {
            log::print(this->s_stream, this->kind, this->file, this->caller, this->line);
        }

        template<typename T>
        requires std::is_same_v<T, Level>
        auto operator<<(const T &t) -> _Log & {
            this->kind = t;
            return *this;
        }

        template <typename T>
        requires (!is_mat<T>::value &&
                  !is_vec<T>::value &&
                  !std::is_same_v<T, _End> &&
                  !std::is_same_v<T, Level> &&
                  !std::is_integral_v<T> &&
                  !is_address<T>::value)
        auto operator<<(const T &t) -> _Log & {
            this->s_stream << t << " ";
            return *this;
        }

    private:
        std::string file;
        std::string caller;

#ifdef DEBUG
        Level kind = Level::LOG_LEVEL_DEBUG;
#else
        Level kind = Level::LOG_LEVEL_NORMAL;
#endif
        size_t line;
        std::stringstream s_stream;
    };

    static inline auto out(
            std::string file = __builtin_FILE(),
            std::string caller = __builtin_FUNCTION(),
            const size_t line = __builtin_LINE()) -> _Log {
        return _Log { file, caller, line };
    }

    template <typename ...Args>
    static inline constexpr auto log(
            std::string file,
            std::string caller,
            const u64 line,
            Args ...args) -> void {
        (out(file, caller, line) << ... << std::forward<Args>(args)) << _End{};
    }
}

#define LOG(...) \
    util::log::log(__builtin_FILE(), __builtin_FUNCTION(), __builtin_LINE(), __VA_ARGS__);

#ifdef DEBUG
    #define DEBUG_LOG(...) LOG(__VA_ARGS__)
#else
    #define DEBUG_LOG(...)
#endif

#endif //OPENGL_3D_ENGINE_LOG_H
