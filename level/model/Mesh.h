//
// Created by Luis Ruisinger on 16.02.24.
//

#ifndef OPENGL_3D_ENGINE_MESH_H
#define OPENGL_3D_ENGINE_MESH_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <string>

#include "glm/gtc/matrix_transform.hpp"
#include "../../rendering/Shader.h"

#define TEX_DIFF "texture_diffuse"
#define TEX_SPEC "texture_specular"

namespace Mesh {
    struct Vertex {
        glm::vec3 position;
        //glm::vec3 normal;
        glm::vec2 texCoords;
    };

    struct Texture {
        uint32_t    id;
        std::string type;
        std::string path;
    };

    class Mesh {
    public:
        std::vector<Vertex>   vertices;
        std::vector<uint32_t> indices;
        uint32_t              textureID;

        Mesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices) noexcept;

        Mesh() = delete;
        ~Mesh();

        void
        draw(uint32_t instanceCount);

    private:

        void
        setupMesh();
    };

    class Model {
    public:
        Model(std::vector<Mesh>, std::vector<std::pair<std::string, std::vector<uint8_t>>>&);

    private:
        std::vector<Mesh>    meshes;

        auto loadTexture(std::pair<std::string, std::vector<uint8_t>>&) -> void;
    };
}

#endif //OPENGL_3D_ENGINE_MESH_H
