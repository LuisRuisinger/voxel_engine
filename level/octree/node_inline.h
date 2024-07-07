//
// Created by Luis Ruisinger on 26.05.24.
//

#ifndef OPENGL_3D_ENGINE_NODE_INLINE_H
#define OPENGL_3D_ENGINE_NODE_INLINE_H

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <memory>
#include <array>
#include <vector>
#include <stack>
#include <functional>

#include "node.h"
#include "../../util/aliases.h"
#include "glad/glad.h"
#include "../Model/mesh.h"

#define BASE_SIZE 1

namespace core::level::node_inline {

    /** @brief Mask to extract the x-offset inside the chunk from packed_data */
    static constexpr const u32 x_and = 0x1F << 13;

    /** @brief Mask to extract the y-offset inside the chunk from packed_data */
    static constexpr const u32 y_and = 0x1F << 8;

    /** @brief Mask to extract the z-offset inside the chunk from packed_data */
    static constexpr const u32 z_and = 0x1F << 3;

    /** @brief Extract the exponent of the scale from packed_data */
    static constexpr const u64 exponent_and = 0x700000000;

    /** @brief Mask packed_data with exponent threshold */
    static constexpr const u64 exponent_check = static_cast<u64>(2) << 32;

    /** @brief Extract everything of packed_data except the highest 14 bit (segment, faces) */
    static constexpr const u64 save_and = 0x03FFFFFFFFFFFF;

    /** @brief Each octree child consists of an extractable mask to store packed inside an u8 */
    static constexpr const u8 index_to_segment[8] = {
            0b10000000U, 0b00001000U, 0b00010000U, 0b00000001U,
            0b01000000U, 0b00000100U, 0b00100000U, 0b00000010U
    };

    /** @brief Scalars used as sign prefix to construct an offset from the current position */
    static constexpr const i8 index_to_prefix[8][3] = {
            {-1, -1, -1}, {-1, -1, 1}, {-1, 1, -1}, {-1, 1,  1},
            { 1, -1, -1}, { 1, -1, 1}, { 1, 1, -1}, { 1, 1,  1}
    };

    /**
     * @brief  Selecting a child from the current node.
     * @param  packed_voxel      The high 32 bit of the voxel containing the position.
     * @param  packed_data_high The high 32 bit of the current node containing position and scale.
     * @return A u8 mask for the chosen child of the current node.
     */
    INLINE static
    auto selectChild(u32 packed_voxel, u32 packed_data_high) -> u8 {
        return (((packed_voxel & x_and) >= (packed_data_high & x_and)) << 2) |
               (((packed_voxel & y_and) >= (packed_data_high & y_and)) << 1) |
                ((packed_voxel & z_and) >= (packed_data_high & z_and));
    }

    /**
     * @brief  Building a cubic box enclosing a certain section of the octree
     * @param  childMask  The targeted child of the curret node
     * @param  packedData The high 32 bit of the current node containing position and scale
     * @return A u32 mask equal to the packed data's high 32 bit of a node
     */
    INLINE static
    auto buildBbox(u8 childMask, u32 packedData) -> u32 {
        auto [pX, pY, pZ] = index_to_prefix[childMask];

        // calculating 2^(n + 1) / 2^2 = 2^n / 2 = 2^(n - 1)
        // n + 1 because we need to fake a 32^3 segment as 64^3 to never need floats
        u8 scale  = (1 << (packedData & 0x7)) >> 1;

        u8 x = ((packedData >> 13) & 0x1F) << 1;
        u8 y = ((packedData >>  8) & 0x1F) << 1;
        u8 z = ((packedData >>  3) & 0x1F) << 1;

        x = static_cast<i8>(x) + scale * pX;
        y = static_cast<i8>(y) + scale * pY;
        z = static_cast<i8>(z) + scale * pZ;

        // the new packedData will always have its packed segments set to 0
        // selecting a node will set the respective segment
        return (packedData & (0x3F << 18)) |

               // setting current position
               // because we shiftet the coordinates to the left
               // we can just extract the higher number and shift it less
               ((x & 0x3E) << 12) |
               ((y & 0x3E) <<  7) |
               ((z & 0x3E) <<  2) |

               // the new factor
               ((packedData & 0x7) - 1);
    }

    /**
     * @brief  Searches for a specific voxel via its position
     * @param  position The voxel position compressed in a u16
     * @param  current  A pointer referring to the current node
     * @return A std::optional containing either the voxel or none
     */
    inline static
    auto findNode(u32 packed_voxel, node::Node *current) -> std::optional<node::Node *> {
        for(;;) {
            if (!(current->packed_data >> 56)) {

                // spherical approximation of the position
                // via distance between the position and current node
                if (((current->packed_data >> 50) & 0x3F)) {
                    auto posVec3 = glm::vec3 {
                            (packed_voxel >> 13) & 0x1F,
                            (packed_voxel >> 8) & 0x1F,
                            (packed_voxel >> 3) & 0x1F
                    };

                    auto rootVec3 = glm::vec3 {
                            (current->packed_data >> 45) & 0x1F,
                            (current->packed_data >> 40) & 0x1F,
                            (current->packed_data >> 35) & 0x1F
                    };

                    u8 scale = 1 << ((current->packed_data >> 32) & 0x7);
                    if ((std::pow(glm::distance(posVec3, rootVec3), 2) * 2) <= std::pow(scale, 2))
                        return std::make_optional(current);
                }
                else {
                    return std::nullopt;
                }
            }

            // if the segment is not in use the voxel doesn't exist
            auto index = selectChild(packed_voxel, current->packed_data >> 32);
            if (!((current->packed_data >> 56) & index_to_segment[index]))
                return std::nullopt;

            current = &current->nodes[index];
        }
    }

    /**
     * @brief Inserts a specific voxel via its position
     *
     * Calculates the position of the voxel inside the tree via using the u32 higher half of
     * the last node's (or root's) packed_data packed data.
     * The high 16 bit of the lower 32 bit of packed_data will be set with the packed_voxel high 16 bit of
     * the lower 32 bit to contain the chunk information.
     * The lowest 16 bit of packed_data will be set to 0 unless the node becomes a voxel.
     *
     * @param  packed_voxel The voxel compressed in a u64
     * @param  data         The current bounding box of the last node (or root of the tree)
     * @return The address of inserted Voxel
     */
    inline
    auto insertNode(u64 packed_voxel, u32 data, node::Node *current) -> node::Node * {
        for(;;) {
            if ((1 << (data & 0x7)) == BASE_SIZE) {

                // setting all faces to visible
                // shifting octree local data to the higher 32 bit
                // adding the voxel unique data
                current->packed_data = (static_cast<u64>(0x3F) << 50) |
                                       (static_cast<u64>(data) << 32) |
                                       (packed_voxel & UINT32_MAX);
                return current;
            }
            else {
                u8 index    = selectChild(packed_voxel >> 32, data);
                u8 segment  = index_to_segment[index];
                u8 segments = current->packed_data >> 56;

                // if the node does not contain any children
                if (!segments) {

                    // initializing the current node's packed data field with
                    // segments, faces, position and high 16 bit of the packed_voxel containing chunk data
                    // lowest 16 bit are set to 0
                    current->packed_data = (static_cast<u64>(segment) << 56) |
                                           (static_cast<u64>(data) << 32) |
                                           (packed_voxel & 0xFFFF0000);


                    current->nodes = util::tagged_ptr::make_tagged<node::Node [8], u16>();
                    ASSERT(current->nodes.get<node::Node>());
                }

                // if the chosen segment is not in use
                if (!(segments & segment))
                    current->packed_data |= (static_cast<u64>(segment) << 56) |
                                            (static_cast<u64>(0x3F) << 50);

                ASSERT(&current->nodes[0])
                data = buildBbox(index, data);
                current    = &current->nodes[index];
            }
        }
    }
}

#endif //OPENGL_3D_ENGINE_NODE_INLINE_H

