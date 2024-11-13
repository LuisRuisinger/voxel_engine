//
// Created by Luis Ruisinger on 16.02.24.
//

#ifndef OPENGL_3D_ENGINE_MESH_H
#define OPENGL_3D_ENGINE_MESH_H

#include <string>

#include "glm/gtc/matrix_transform.hpp"
#include "../../../util/defines.h"

#define TEX_DIFF "texture_diffuse"
#define TEX_SPEC "texture_specular"

namespace Mesh {
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoords;
        u8 texture_offset;
    };

    struct Face {
        std::vector<u64> vertices;
    };

    struct Texture {
        uint32_t    id;
        std::string type;
        std::string path;
    };
}

#endif //OPENGL_3D_ENGINE_MESH_H
