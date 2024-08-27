//
// Created by Luis Ruisinger on 03.02.24.
//

#include "shader.h"
#include "../../util/log.h"

namespace core::rendering::shader {
    auto Shader::init(
            const char *vertex_path, 
            const char *fragment_path) ->  Result<void, Error> {
        std::ifstream vertex_shader_ifstream;
        std::ifstream fragment_shader_ifstream;

        std::filesystem::path base_path = std::filesystem::path(__FILE__).parent_path();
        std::filesystem::path full_vertex_path = base_path / "shaders" / vertex_path;
        std::filesystem::path full_fragment_path = base_path / "shaders" / fragment_path;

        vertex_shader_ifstream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fragment_shader_ifstream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try {
            vertex_shader_ifstream.open(full_vertex_path);
            fragment_shader_ifstream.open(full_fragment_path);

            std::stringstream vertex_shader_sstream, fragment_shader_sstream;

            vertex_shader_sstream << vertex_shader_ifstream.rdbuf();
            fragment_shader_sstream << fragment_shader_ifstream.rdbuf();

            vertex_shader_ifstream.close();
            fragment_shader_ifstream.close();

            std::string vertexCode = vertex_shader_sstream.str();
            std::string fragmentCode = fragment_shader_sstream.str();

            const char *vertex_shader_code = vertexCode.c_str();
            const char *fragment_shader_code = fragmentCode.c_str();

            i32  success;
            char infoLog[512];

            // vertex Shader
            u32 vertex = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vertex, 1, &vertex_shader_code, nullptr);
            glCompileShader(vertex);

            glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
            if(!success) {
                glGetShaderInfoLog(vertex, 512, nullptr, (char *) infoLog);
                LOG(util::log::LOG_LEVEL_ERROR, reinterpret_cast<char *>(infoLog));
                return Err(Error::VERTEX_SHADER_COMPILATION_FAILED);
            }

            // fragment Shader
            u32 fragment = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fragment, 1, &fragment_shader_code, nullptr);
            glCompileShader(fragment);

            glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
            if(!success) {
                glGetShaderInfoLog(fragment, 512, nullptr, (char *) infoLog);
                LOG(util::log::LOG_LEVEL_ERROR, reinterpret_cast<char *>(infoLog));
                return Err(Error::FRAGMENT_SHADER_COMPILATION_FAILED);
            }

            // shader Program
            this->id = glCreateProgram();
            glAttachShader(this->id, vertex);
            glAttachShader(this->id, fragment);
            glLinkProgram(this->id);

            // print linking errors if any
            glGetProgramiv(this->id, GL_LINK_STATUS, &success);
            if(!success) {
                glGetProgramInfoLog(this->id, 512, nullptr, (char *) infoLog);
                LOG(util::log::LOG_LEVEL_ERROR, reinterpret_cast<char *>(infoLog));
                return Err(Error::SHADER_LINKING_FAILED);
            }

            // delete the shaders as they're linked into our program now and no longer necessary
            glDeleteShader(vertex);
            glDeleteShader(fragment);
        }
        catch(std::ifstream::failure &err) {
            return Err(Error::FILE_STREAM_ERROR);
        }

        return Ok();
    }

    auto Shader::use() const -> void {
        glUseProgram(this->id);
    }

    auto Shader::registerUniformLocation(std::string name) const -> void {
        auto location = glGetUniformLocation(this->id, name.c_str());
        this->uniform_cache[name] = location;
    }

    auto Shader::operator[](std::string name) const -> Impl & {
        return this->uniform_values[name];
    }

    auto Shader::upload_uniforms() const -> void {
        for (auto &[k, v] : this->uniform_values)
            std::visit(overload {
                [&](bool &arg) {
                    glUniform1i(this->uniform_cache[k], arg); },
                [&](i32 &arg) {
                    glUniform1i(this->uniform_cache[k], arg); },
                [&](u32 &arg) {
                    glUniform1ui(this->uniform_cache[k], arg); },
                [&](f32 &arg) {
                    glUniform1f(this->uniform_cache[k], arg); },
                [&](glm::vec2 &arg) {
                    glUniform2f(this->uniform_cache[k], arg.x, arg.y); },
                [&](glm::vec3 &arg) {
                    glUniform3f(this->uniform_cache[k], arg.x, arg.y, arg.z); },
                [&](glm::mat4 &arg) {
                    glUniformMatrix4fv(uniform_cache[k], 1, GL_FALSE, glm::value_ptr(arg)); }
                }, v);
    }
}