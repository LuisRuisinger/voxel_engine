//
// Created by Luis Ruisinger on 16.02.24.
//

#include "mesh.h"

#include <utility>

namespace Mesh {
    Mesh::Mesh(std::vector<Vertex> vertices,
               std::vector<uint32_t> indices) noexcept {
        this->vertices = std::move(vertices);
        this->indices  = std::move(indices);

        setupMesh();
    }

    Mesh::~Mesh() {
        // glDeleteVertexArrays(1, &this->VAO);
        // glDeleteBuffers(1,      &this->VBO);
        // glDeleteBuffers(1,      &this->EBO);
    }

    void Mesh::setupMesh() {

    }

    static constexpr unsigned int strtint(std::string_view str, uint32_t h = 0) {
        return !str[h] ? 5381 : (strtint(str, h + 1) * 33) ^ str[h];
    }

    void Mesh::draw(uint32_t instanceCount) {

    }
}
