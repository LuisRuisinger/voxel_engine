//
// Created by Luis Ruisinger on 03.02.24.
//

#include "Shader.h"

Shader::Shader(const char *vertexPath, const char *fragmentPath) {
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;

    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);

        std::stringstream vShaderStream, fShaderStream;

        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();

        vShaderFile.close();
        fShaderFile.close();

        std::string vertexCode   = vShaderStream.str();
        std::string fragmentCode = fShaderStream.str();
        const char* vShaderCode  = vertexCode.c_str();
        const char* fShaderCode  = fragmentCode.c_str();

        i32 success;
        c8  infoLog[512];

        // vertex Shader
        u32 vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, nullptr);
        glCompileShader(vertex);

        glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
        if(!success) {
            glGetShaderInfoLog(vertex, 512, nullptr, (char *) infoLog);
            std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << (char *) infoLog << std::endl;
        }

        // fragment Shader
        u32 fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, nullptr);
        glCompileShader(fragment);

        glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
        if(!success) {
            glGetShaderInfoLog(fragment, 512, nullptr, (char *) infoLog);
            std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << (char *) infoLog << std::endl;
        }

        // shader Program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);

        // print linking errors if any
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if(!success)
        {
            glGetProgramInfoLog(ID, 512, nullptr, (char *) infoLog);
            std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << (char *) infoLog << std::endl;
        }

        // delete the shaders as they're linked into our program now and no longer necessary
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }
    catch(std::ifstream::failure &err) {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
    }
}

auto Shader::use() -> void {
    glUseProgram(this->ID);
}

auto Shader::setBool(const std::string &name, bool value) const -> void {
    glUniform1i(glGetUniformLocation(this->ID, name.c_str()), value);
}

auto Shader::setInt(const std::string &name, i32 value) const -> void {
    glUniform1i(glGetUniformLocation(this->ID, name.c_str()), value);
}

auto Shader::setFloat(const std::string &name, f32 value) const -> void {
    glUniform1f(glGetUniformLocation(this->ID, name.c_str()), value);
}

auto Shader::set_vec3(const std::string &name, f32 x, f32 y, f32 z) const -> void {
    glUniform3f(glGetUniformLocation(this->ID, name.c_str()), x, y, z);
}