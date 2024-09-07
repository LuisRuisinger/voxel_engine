//
// Created by Luis Ruisinger on 14.03.24.
//

#include "voxel.h"
#include "../../../util/log.h"

#define POS_CONV(_p) \
    (static_cast<u64>(((_p) + 0.5F) * 4.0F) & 0x7)

#define UV_CONV(_p)  \
    (static_cast<u64>((_p) * 2.0F) & 0x3)

namespace core::level::model::voxel {
    CubeStructure::CubeStructure() {
        std::vector<std::vector<Mesh::Vertex>> mesh = {

                // Face 0: Back
                {
                    {{0.5f,  -0.5f, -0.5f}, {0.0f, 0.0f}, 2},   // bottom right
                    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, 2},  // bottom left
                    {{-0.5f, 0.5f,  -0.5f}, {1.0f, 1.0f}, 2},   // top left
                    {{0.5f,  0.5f,  -0.5f}, {0.0f, 1.0f}, 2}     // top right
                },

                // Face 1: Front
                {
                    {{-0.5f, -0.5f, 0.5f},  {0.0f, 0.0f}, 1},  // bottom left
                    {{0.5f,  -0.5f, 0.5f},  {1.0f, 0.0f}, 1},   // bottom right
                    {{0.5f,  0.5f,  0.5f},  {1.0f, 1.0f}, 1},    // top right
                    {{-0.5f, 0.5f,  0.5f},  {0.0f, 1.0f}, 1}    // top left
                },

                // Face 2: Bottom
                {
                    {{0.5f,  -0.5f, 0.5f},  {0.0f, 0.0f}, 3},    // bottom right
                    {{-0.5f, -0.5f, 0.5f},  {1.0f, 0.0f}, 3},   // bottom left
                    {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f}, 3},  // top left
                    {{0.5f,  -0.5f, -0.5f}, {0.0f, 1.0f}, 3},     // top right
                },

                // Face 3: Top
                {
                    {{-0.5f, 0.5f,  0.5f},  {0.0f, 1.0f}, 0},     // bottom left
                    {{0.5f,  0.5f,  0.5f},  {1.0f, 1.0f}, 0},     // bottom right
                    {{0.5f,  0.5f,  -0.5f}, {1.0f, 0.0f}, 0},    // top right
                    {{-0.5f, 0.5f,  -0.5f}, {0.0f, 0.0f}, 0},   // top left
                },

                // Face 4: Right
                {
                    {{0.5f,  0.5f,  -0.5f}, {0.0f, 1.0f}, 2},   // top front
                    {{0.5f,  0.5f,  0.5f},  {1.0f, 1.0f}, 2},     // top back
                    {{0.5f,  -0.5f, 0.5f},  {1.0f, 0.0f}, 2},    // bottom back
                    {{0.5f,  -0.5f, -0.5f}, {0.0f, 0.0f}, 2},   // bottom front
                },

                // Face 5: Left
                {
                    {{-0.5f, 0.5f,  0.5f},  {0.0f, 1.0f}, 2},     // top front
                    {{-0.5f, 0.5f,  -0.5f}, {1.0f, 1.0f}, 2},   // top back
                    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, 2},  // bottom back
                    {{-0.5f, -0.5f, 0.5f},  {0.0f, 0.0f}, 2},  // bottom front
                }
        };

        for (size_t i = 0; i < mesh.size(); ++i)
            compress_face(mesh[i], i);
    }

    auto CubeStructure::compress_face(std::vector<Mesh::Vertex> &face, u8 face_idx) -> void {
        std::vector<u64> faces(face.size());

        for (size_t i = 0; i < face.size(); ++i) {
            faces[i] = (POS_CONV(face[i].position.x) << 61) |
                       (POS_CONV(face[i].position.y) << 58) |
                       (POS_CONV(face[i].position.z) << 55) |

                       // compressing uv coordinates
                       (UV_CONV(face[i].texCoords.x) << 53) |
                       (UV_CONV(face[i].texCoords.y) << 51) |

                       // compressing texture offset
                       (face[i].texture_offset << 11);
        }

#ifdef __AVX2__
        __m256i combined = _mm256_loadu_si256(
                reinterpret_cast<__m256i const *>(faces.data()));
        _mm256_storeu_si256(&this->compressed_faces[face_idx], combined);
#else
        this->compressed_faces[face_idx] = std::move(faces);
#endif
    }

    auto CubeStructure::mesh() const  -> const Compressed & {
        return this->compressed_faces;
    }
}