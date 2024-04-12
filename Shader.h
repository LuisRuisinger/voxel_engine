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

#include "global.h"

class Shader {
public:
    u32 ID;

    Shader(const char *vertexPath, const char *fragmentPath);

    auto use() -> void;

    auto registerUniformLocation(std::string name) const -> void;

    auto setBool(std::string name, bool value) const -> void;
    auto setInt(std::string name, int value) const -> void;
    auto setFloat(std::string name, float value) const -> void;
    auto setVec3(std::string name, vec3f vec) const -> void;
    auto setMat4(std::string name, glm::mat4& mat) -> void;

private:
    std::unordered_map<std::string, GLint> mutable uniformCache;
};

#endif //OPENGL_3D_ENGINE_SHADER_H
