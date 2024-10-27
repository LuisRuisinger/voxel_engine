//
// Created by Luis Ruisinger on 03.02.24.
//

#ifndef OPENGL_3D_ENGINE_SHADER_H
#define OPENGL_3D_ENGINE_SHADER_H

#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include "../core/opengl/opengl_verify.h"

#include "../util/defines.h"
#include "../util/result.h"
#include "../util/log.h"

namespace core::rendering::shader {

    enum Error : u8 {
        VERTEX_SHADER_COMPILATION_FAILED,
        GEOMETRY_SHADER_COMPILATION_FAILED,
        FRAGMENT_SHADER_COMPILATION_FAILED,
        SHADER_LINKING_FAILED,
        FILE_STREAM_ERROR,
        SHADER_INIT_ERROR
    };

    enum Type {
        VERTEX_SHADER, GEOMETRY_SHADER, FRAGMENT_SHADER
    };

    template <Type T>
    class Shader {
    public:
        Shader(const char *path)
            : full_path { std::filesystem::path(__FILE__).parent_path() / "shaders" / path }
        {}

        auto init() -> Result<u32, Error> {
            auto file_stream = std::ifstream {};
            file_stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

            try {
                file_stream.open(this->full_path);

                auto s_stream = std::stringstream {};
                s_stream << file_stream.rdbuf();

                u32 shader;
                if constexpr (T == VERTEX_SHADER)
                    shader = glCreateShader(GL_VERTEX_SHADER);

                if constexpr (T == GEOMETRY_SHADER)
                    shader = glCreateShader(GL_GEOMETRY_SHADER);

                if constexpr (T == FRAGMENT_SHADER)
                    shader = glCreateShader(GL_FRAGMENT_SHADER);

                const std::string &tmp = s_stream.str();
                const char *cstr = tmp.c_str();
                OPENGL_VERIFY(glShaderSource(shader, 1, &cstr, nullptr));
                OPENGL_VERIFY(glCompileShader(shader));

                i32 success;
                OPENGL_VERIFY(glGetShaderiv(shader, GL_COMPILE_STATUS, &success));

                char info_log[512];
                if(!success) {
                    OPENGL_VERIFY(glGetShaderInfoLog(
                            shader, 512, nullptr, reinterpret_cast<char *>(info_log)));
                    LOG(util::log::LOG_LEVEL_ERROR, reinterpret_cast<char *>(info_log));

                    OPENGL_VERIFY(glDeleteShader(shader));
                    if constexpr (T == VERTEX_SHADER)
                        return Err(Error::VERTEX_SHADER_COMPILATION_FAILED);

                    if constexpr (T == GEOMETRY_SHADER)
                        return Err(Error::GEOMETRY_SHADER_COMPILATION_FAILED);

                    if constexpr (T == FRAGMENT_SHADER)
                        return Err(Error::FRAGMENT_SHADER_COMPILATION_FAILED);
                }

                return Ok(shader);
            }
            catch(std::ifstream::failure &err) {
                return Err(Error::FILE_STREAM_ERROR);
            }
        }

    private:
        std::filesystem::path full_path;
    };

    template<typename ...Ts>
    struct overload : Ts... { using Ts::operator()...; };

    template<typename ...Ts>
    overload(Ts...) -> overload<Ts...>;

    using Impl = std::variant<bool, i32, f32, glm::vec2, glm::vec3, glm::mat4, u32>;

    class Program {
    public:
        Program() =default;

        template <typename ...Args>
        auto init(Args ...args) -> Result<void, Error> {
            auto tuple = std::make_tuple(args.init() ...);
            auto is_err = std::apply(
                    [](const auto &...args) -> bool { return (... || args.isErr()); },
                    tuple);

            if (is_err) {
                std::apply([](auto &...args) {
                    auto delete_if_no_error = [](auto &shader) {
                        if (!shader.isErr()) {
                            OPENGL_VERIFY(glDeleteShader(shader.unwrap()));
                        }
                    };

                    (delete_if_no_error(args), ...);
                }, tuple);

                return Err(Error::SHADER_INIT_ERROR);
            }

            this->id = glCreateProgram();
            std::apply(
                    [this](const auto &...args) -> void {
                        (glAttachShader(this->id, args.unwrap()), ...); },
                    tuple);

            i32 success;
            char info_log[512];

            OPENGL_VERIFY(glLinkProgram(this->id));
            OPENGL_VERIFY(glGetProgramiv(this->id, GL_LINK_STATUS, &success));

            std::apply(
                    [](const auto &...args) -> void {
                        (glDeleteShader(args.unwrap()), ...); },
                    tuple);

            if(!success) {
                OPENGL_VERIFY(glGetProgramInfoLog(
                        this->id, 512, nullptr, reinterpret_cast<char *>(info_log)));
                LOG(util::log::LOG_LEVEL_ERROR, reinterpret_cast<char *>(info_log));

                return Err(Error::SHADER_LINKING_FAILED);
            }

            return Ok();
        }

        auto use() const -> void;
        auto register_uniform(std::string) const -> void;
        auto operator[](std::string) const -> Impl &;
        auto upload_uniforms() const -> void;
        auto shader_id() -> u32;

    private:
        u32 id;
        mutable std::unordered_map<std::string, GLint> uniform_cache;
        mutable std::unordered_map<std::string, Impl> uniform_values;
    };

}
#endif //OPENGL_3D_ENGINE_SHADER_H
