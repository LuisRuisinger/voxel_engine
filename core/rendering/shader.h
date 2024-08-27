//
// Created by Luis Ruisinger on 03.02.24.
//

#ifndef OPENGL_3D_ENGINE_SHADER_H
#define OPENGL_3D_ENGINE_SHADER_H

#include "glad/glad.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include "../../util/aliases.h"
#include "../../util/result.h"

namespace core::rendering::shader {

    enum Error : u8 {
        VERTEX_SHADER_COMPILATION_FAILED,
        FRAGMENT_SHADER_COMPILATION_FAILED,
        SHADER_LINKING_FAILED,
        FILE_STREAM_ERROR
    };

    template<typename ...Ts>
    struct overload : Ts... { using Ts::operator()...; };

    template<typename ...Ts>
    overload(Ts...) -> overload<Ts...>;

    using Impl = std::variant<bool, i32, f32, glm::vec2, glm::vec3, glm::mat4, u32>;

    class Shader {
    public:
        Shader() =default;

        auto init(const char *, const char *) -> Result<void, Error>;
        auto use() const -> void;
        auto registerUniformLocation(std::string) const -> void;
        auto operator[](std::string) const -> Impl &;
        auto upload_uniforms() const -> void;

    private:
        u32 id;
        mutable std::unordered_map<std::string, GLint> uniform_cache;
        mutable std::unordered_map<std::string, Impl> uniform_values;
    };

}
#endif //OPENGL_3D_ENGINE_SHADER_H
