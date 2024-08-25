//
// Created by Luis Ruisinger on 14.03.24.
//

#include "voxel.h"

#define POS_CONV(_p) \
    (static_cast<u64>(((_p) + 0.5F) * 4.0F) & 0x7)

#define UV_CONV(_p)  \
    (static_cast<u64>((_p) * 2.0F) & 0x3)

namespace core::level::model::voxel {
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
        std::vector<u64> compressedFace(face.size());

        for (size_t i = 0; i < face.size(); ++i)

            // constructing compressed faces from n vertices
            compressedFace[i] = (POS_CONV(face[i].position.x) << 61) |
                                (POS_CONV(face[i].position.y) << 58) |
                                (POS_CONV(face[i].position.z) << 55) |

                                // compressing uv coordinates
                                (UV_CONV(face[i].texCoords.x) << 53) |
                                (UV_CONV(face[i].texCoords.y) << 51);

        _compressedFaces[idx] = std::move(compressedFace);
    }

    auto CubeStructure::mesh() const  -> const std::array<std::vector<u64>, 6> & {
        return _compressedFaces;
    }
}