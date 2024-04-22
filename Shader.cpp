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
        if(!success) {
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

auto Shader::registerUniformLocation(std::string name) const -> void {
    auto location = glGetUniformLocation(this->ID, name.c_str());
    this->uniformCache[name] = location;
}

auto Shader::setBool(std::string name, bool value) const -> void {
    glUniform1i(uniformCache[name], value);
}

auto Shader::setInt(std::string name, i32 value) const -> void {
    glUniform1i(uniformCache[name], value);
}

auto Shader::setFloat(std::string name, f32 value) const -> void {
    glUniform1f(uniformCache[name], value);
}

auto Shader::setVec2(std::string name, vec2f vec) const -> void {
    glUniform2f(uniformCache[name], vec.x, vec.y);
}

auto Shader::setVec3(std::string name, vec3f vec) const -> void {
    glUniform3f(uniformCache[name], vec.x, vec.y, vec.z);
}

auto Shader::setMat4(std::string name, glm::mat4 &mat) -> void {
    glUniformMatrix4fv(uniformCache[name], 1, GL_FALSE, glm::value_ptr(mat));
}