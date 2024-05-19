//
// Created by Luis Ruisinger on 14.03.24.
//

#include "Voxel.h"

namespace VoxelStructure {
    CubeStructure::CubeStructure()
        : _compressedFaces{}
    {
        std::vector<std::vector<Mesh::Vertex>> mesh = {

                // Face 0: Back
                {
                    {{0.5f,  -0.5f, -0.5f}, {0.0f, 0.0f}},   // bottom right
                    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}},  // bottom left
                    {{-0.5f, 0.5f,  -0.5f}, {1.0f, 1.0f}},   // top left
                    {{0.5f,  0.5f,  -0.5f}, {0.0f, 1.0f}}     // top right
                },

                // Face 1: Front
                {
                    {{-0.5f, -0.5f, 0.5f},  {0.0f, 0.0f}},  // bottom left
                    {{0.5f,  -0.5f, 0.5f},  {1.0f, 0.0f}},   // bottom right
                    {{0.5f,  0.5f,  0.5f},  {1.0f, 1.0f}},    // top right
                    {{-0.5f, 0.5f,  0.5f},  {0.0f, 1.0f}}    // top left
                },

                // Face 2: Bottom
                {
                    {{0.5f,  -0.5f, 0.5f},  {0.0f, 0.0f}},    // bottom right
                    {{-0.5f, -0.5f, 0.5f},  {1.0f, 0.0f}},   // bottom left
                    {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f}},  // top left
                    {{0.5f,  -0.5f, -0.5f}, {0.0f, 1.0f}},     // top right
                },

                // Face 3: Top
                {
                    {{-0.5f, 0.5f,  0.5f},  {0.0f, 1.0f}},     // bottom left
                    {{0.5f,  0.5f,  0.5f},  {1.0f, 1.0f}},     // bottom right
                    {{0.5f,  0.5f,  -0.5f}, {1.0f, 0.0f}},    // top right
                    {{-0.5f, 0.5f,  -0.5f}, {0.0f, 0.0f}},   // top left
                },

                // Face 4: Right
                {
                    {{0.5f,  0.5f,  -0.5f}, {0.0f, 1.0f}},   // top front
                    {{0.5f,  0.5f,  0.5f},  {1.0f, 1.0f}},     // top back
                    {{0.5f,  -0.5f, 0.5f},  {1.0f, 0.0f}},    // bottom back
                    {{0.5f,  -0.5f, -0.5f}, {0.0f, 0.0f}},   // bottom front
                },

                // Face 5: Left
                {
                    {{-0.5f, 0.5f,  0.5f},  {0.0f, 1.0f}},     // top front
                    {{-0.5f, 0.5f,  -0.5f}, {1.0f, 1.0f}},   // top back
                    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}},  // bottom back
                    {{-0.5f, -0.5f, 0.5f},  {0.0f, 0.0f}},  // bottom front
                }
        };

        for (size_t i = 0; i < mesh.size(); ++i)
            setFace(mesh[i], i);
    }

    auto CubeStructure::setFace(std::vector<Mesh::Vertex> &face, u8 idx) -> void {

        // this can be inversed through computing: (value * 0.25F) - 0.5F
        std::unordered_map<float_t, u64> posConv = {
                {-0.5F, 0b000}, {-0.25F, 0b001}, {0.0F, 0b010}, {0.25F, 0b011}, {0.5F, 0b100}
        };

        // this can be inversed through computing: value * 0.5F
        std::unordered_map<float_t, u64> uvConv = {
                {0.0F, 0b00}, {0.5F, 0b01}, {1.0F, 0b10}
        };

        std::vector<u64> compressedFace(face.size());
        for (size_t i = 0; i < face.size(); ++i)

            // constructing compressed faces from n vertices
            compressedFace[i] = (posConv[face[i].position.x] << 61) |
                                (posConv[face[i].position.y] << 58) |
                                (posConv[face[i].position.z] << 55) |

                                // compressing uv coordinates
                                (uvConv[face[i].texCoords.x] << 53) |
                                (uvConv[face[i].texCoords.y] << 51);

        _compressedFaces[idx] = compressedFace;
    }

    auto CubeStructure::mesh() const  -> const std::array<std::vector<u64>, 6> & {
        return _compressedFaces;
    }
}