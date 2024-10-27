//
// Created by Luis Ruisinger on 03.02.24.
//

#include "shader.h"
#include "../../util/log.h"

namespace core::rendering::shader {
    auto Program::use() const -> void {
        glUseProgram(this->id);
    }

    auto Program::register_uniform(std::string name) const -> void {
        auto location = glGetUniformLocation(this->id, name.c_str());
        this->uniform_cache[name] = location;
    }

    auto Program::operator[](std::string name) const -> Impl & {
        return this->uniform_values[name];
    }

    auto Program::upload_uniforms() const -> void {
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

    auto Program::shader_id() -> u32 {
        return this->id;
    }
}