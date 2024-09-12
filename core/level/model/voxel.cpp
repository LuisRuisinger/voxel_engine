//
// Created by Luis Ruisinger on 14.03.24.
//

#include "voxel.h"
#include "../../../util/log.h"

#define POS_CONV(_p) \
    (static_cast<u64>(((_p) + 0.5F) * 4.0F) & 0x7)

#define UV_CONV(_p) \
    (static_cast<u64>((_p) * 2.0F) & 0x3)

namespace core::level::model::voxel {
    CubeStructure::CubeStructure() {
        std::vector<std::vector<Mesh::Vertex>> mesh = {

                // face 0: back
                {
                    { {  0.5F, -0.5F, -0.5F }, { 0.0F, 0.0F, -1.0F }, { 0.0F, 0.0F }, 2 }, // bottom right
                    { { -0.5F, -0.5F, -0.5F }, { 0.0F, 0.0F, -1.0F }, { 1.0F, 0.0F }, 2 }, // bottom left
                    { { -0.5F,  0.5F, -0.5F }, { 0.0F, 0.0F, -1.0F }, { 1.0F, 1.0F }, 2 }, // top left
                    { {  0.5F,  0.5F, -0.5F }, { 0.0F, 0.0F, -1.0F }, { 0.0F, 1.0F }, 2 }  // top right
                },

                // face 1: front
                {
                    { { -0.5F, -0.5F, 0.5F }, { 0.0F, 0.0F, 1.0F }, { 0.0F, 0.0F }, 1 }, // bottom left
                    { {  0.5F, -0.5F, 0.5F }, { 0.0F, 0.0F, 1.0F }, { 1.0F, 0.0F }, 1 }, // bottom right
                    { {  0.5F,  0.5F, 0.5F }, { 0.0F, 0.0F, 1.0F }, { 1.0F, 1.0F }, 1 }, // top right
                    { { -0.5F,  0.5F, 0.5F }, { 0.0F, 0.0F, 1.0F }, { 0.0F, 1.0F }, 1 }  // top left
                },

                // face 2: bottom
                {
                    { {  0.5F, -0.5F,  0.5F }, { 0.0F, -1.0F, 0.0F }, { 0.0F, 0.0F }, 3 }, // bottom right
                    { { -0.5F, -0.5F,  0.5F }, { 0.0F, -1.0F, 0.0F }, { 1.0F, 0.0F }, 3 }, // bottom left
                    { { -0.5F, -0.5F, -0.5F }, { 0.0F, -1.0F, 0.0F }, { 1.0F, 1.0F }, 3 }, // top left
                    { {  0.5F, -0.5F, -0.5F }, { 0.0F, -1.0F, 0.0F }, { 0.0F, 1.0F }, 3 }, // top right
                },

                // face 3: top
                {
                    { { -0.5F, 0.5F,   0.5F }, { 0.0F, 1.0F, 0.0F }, { 0.0F, 1.0F }, 0 }, // bottom left
                    { {  0.5F, 0.5F,   0.5F }, { 0.0F, 1.0F, 0.0F }, { 1.0F, 1.0F }, 0 }, // bottom right
                    { {  0.5F, 0.5F,  -0.5F }, { 0.0F, 1.0F, 0.0F }, { 1.0F, 0.0F }, 0 }, // top right
                    { { -0.5F, 0.5F,  -0.5F }, { 0.0F, 1.0F, 0.0F }, { 0.0F, 0.0F }, 0 }, // top left
                },

                // face 4: right
                {
                    { { 0.5F,  0.5F, -0.5F }, { 1.0F, 0.0F, 0.0F }, { 0.0F, 1.0F }, 2 }, // top front
                    { { 0.5F,  0.5F,  0.5F }, { 1.0F, 0.0F, 0.0F }, { 1.0F, 1.0F }, 2 }, // top back
                    { { 0.5F, -0.5F,  0.5F }, { 1.0F, 0.0F, 0.0F }, { 1.0F, 0.0F }, 2 }, // bottom back
                    { { 0.5F, -0.5F, -0.5F }, { 1.0F, 0.0F, 0.0F }, { 0.0F, 0.0F }, 2 }, // bottom front
                },

                // face 5: left
                {
                    { { -0.5F,  0.5f,  0.5F }, { -1.0F, 0.0F, 0.0F }, { 0.0F, 1.0F }, 2 }, // top front
                    { { -0.5F,  0.5f, -0.5F }, { -1.0F, 0.0F, 0.0F }, { 1.0F, 1.0F }, 2 }, // top back
                    { { -0.5F, -0.5f, -0.5F }, { -1.0F, 0.0F, 0.0F }, { 1.0F, 0.0F }, 2 }, // bottom back
                    { { -0.5F, -0.5f,  0.5F }, { -1.0F, 0.0F, 0.0F }, { 0.0F, 0.0F }, 2 }, // bottom front
                }
        };

        for (size_t i = 0; i < mesh.size(); ++i)
            compress_face(mesh[i], i);
    }

    inline static
    auto compress_normal(glm::vec3 &normal) -> u8 {
        if (normal.x < 0.0F) return 0b000;
        if (normal.x > 0.0F) return 0b001;
        if (normal.y < 0.0F) return 0b010;
        if (normal.y > 0.0F) return 0b011;
        if (normal.z < 0.0F) return 0b100;
        return 0b101;
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

                       // compressing normal
                       (compress_normal(face[i].normal) << 13) |

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