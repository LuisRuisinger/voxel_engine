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

namespace util::log {
    template<class T>
    struct is_vec {
        static const bool value = false;
    };

    template<glm::length_t L, class T, glm::qualifier Q>
    struct is_vec<glm::vec<L, T, Q>> {
        static const bool value = true;
    };

    template<class T>
    struct is_mat {
        static const bool value = false;
    };

    template<glm::length_t C, glm::length_t R, class T, glm::qualifier Q>
    struct is_mat<glm::mat<C, R, T, Q>> {
        static const bool value = true;
    };

    enum Level {
        LOG_LEVEL_NORMAL,
        LOG_LEVEL_WARN,
        LOG_LEVEL_ERROR,
        LOG_LEVEL_DEBUG
    };

    auto print(
            std::string_view out,
            Level kind = Level::LOG_LEVEL_DEBUG,
            const char *file = __builtin_FILE(),
            const char *caller = __builtin_FUNCTION(),
            const size_t line = __builtin_LINE()) -> void;

    template<typename T>
    requires (!std::is_convertible<T, std::string>::value &&
              !std::is_same<T, std::stringstream>::value)
    auto print(
            const T &out,
            Level kind = Level::LOG_LEVEL_DEBUG,
            const char *file = __builtin_FILE(),
            const char *caller = __builtin_FUNCTION(),
            const size_t line = __builtin_LINE()) -> decltype(std::to_string(out), void()) {
        log::print(std::string_view(out), kind, file, caller, line);
    }

    inline auto print(
            std::stringstream &out,
            Level kind = Level::LOG_LEVEL_DEBUG,
            const char *file = __builtin_FILE(),
            const char *caller = __builtin_FUNCTION(),
            const size_t line = __builtin_LINE()) -> void {
        log::print(out.str(), kind, file, caller, line);
    }

    struct _End {
    };
    static constexpr auto end = _End{};

    struct _Log {
        _Log(const char *file, const char *caller, const size_t line)
                : file{file}, caller{caller}, line{line} {}

        template<typename T>
        requires (is_mat<T>::value || is_vec<T>::value)
        auto operator<<(const T &t) -> _Log & {
            this->s_stream << glm::to_string(t);
            return *this;
        }

        template<typename T>
        requires (!is_mat<T>::value &&
                  !is_vec<T>::value &&
                  !std::is_same_v<T, _End> &&
                  !std::is_same_v<T, Level>)
        auto operator<<(const T &t) -> _Log & {
            this->s_stream << t;
            return *this;
        }

        template<typename T>
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

    private:
        const char *file;
        const char *caller;
        Level kind = Level::LOG_LEVEL_DEBUG;
        size_t line;
        std::stringstream s_stream;
    };

    static inline auto out(
            const char *file = __builtin_FILE(),
            const char *caller = __builtin_FUNCTION(),
            const size_t line = __builtin_LINE()) -> _Log {
        return _Log{file, caller, line};
    }
}

#define LOG(_o) \
    util::log::print((_o), util::log::Level::LOG_LEVEL_DEBUG);
#endif //OPENGL_3D_ENGINE_LOG_H
