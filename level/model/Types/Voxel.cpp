//
// Created by Luis Ruisinger on 14.03.24.
//

#include "Voxel.h"

namespace VoxelStructure {
    CubeStructure::CubeStructure() {

        // Face 0: Back
        std::vector<Mesh::Vertex> face0 = {
                {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},   // bottom right
                {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}},  // bottom left
                {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f}},   // top left
                {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f}}     // top right
        };

        // Face 1: Front
        std::vector<Mesh::Vertex> face1 = {
                {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f}},  // bottom left
                {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f}},   // bottom right
                {{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f}},    // top right
                {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f}}    // top left
        };

        // Face 2: Bottom
        std::vector<Mesh::Vertex> face2 = {
                {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f}},    // bottom right
                {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f}},   // bottom left
                {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f}},  // top left
                {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}},     // top right
        };

        // Face 3: Top
        std::vector<Mesh::Vertex> face3 = {
                {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f}},     // bottom left
                {{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f}},     // bottom right
                {{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f}},    // top right
                {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f}},   // top left
        };

        // Face 4: Right
        std::vector<Mesh::Vertex> face4 = {
                {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f}},   // top front
                {{0.5f, 0.5f,  0.5f},  {1.0f, 1.0f}},     // top back
                {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f}},    // bottom back
                {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},   // bottom front
        };

        // Face 5: Left
        std::vector<Mesh::Vertex> face5 = {
                {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f}},     // top front
                {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f}},   // top back
                {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}},  // bottom back
                {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f}},  // bottom front
        };

        std::vector<uint32_t> indices = {
                0, 1, 3,
                1, 2, 3
        };

        this->faces = {{face0, indices}, {face1, indices}, {face2, indices},
                       {face3, indices}, {face4, indices}, {face5, indices}};
    }

    auto CubeStructure::structure() const -> const std::vector<Mesh::Mesh> & {
        return this->faces;
    }
}