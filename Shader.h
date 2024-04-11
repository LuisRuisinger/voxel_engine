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
    auto setBool(const std::string& name, bool value) const -> void;
    auto setInt(const std::string& name, int value) const -> void;
    auto setFloat(const std::string& name, float value) const -> void;
    auto set_vec3(const std::string &name, float x, float y, float z) const -> void;
};

#endif //OPENGL_3D_ENGINE_SHADER_H
