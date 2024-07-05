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

    // masks to extract (x, y, z) offsets inside the chunk from _packed
    static constexpr const u32 xAnd           = (0x1F << 13);
    static constexpr const u32 yAnd           = (0x1F <<  8);
    static constexpr const u32 zAnd           = (0x1F <<  3);

    // extract the exponent of the scale from _packed
    static constexpr const u64 exponent_and   = 0x700000000;

    // directly able to mask _packed with an exponent threshold of 1 << 2 = 4
    // volumes of this size won't be tested against the frustum because drawing is cheaper
    static constexpr const u64 exponent_check = static_cast<u64>(2) << 32;

    // extract everything of _packed except the highest 14 bit (segment, faces)
    static constexpr const u64 save_and       = 0x03FFFFFFFFFFFF;

    // each octree child consists of an extractable mask to store packed inside a u8
    static constexpr const u8 indexToSegment[8] = {
            0b10000000U, 0b00001000U, 0b00010000U, 0b00000001U,
            0b01000000U, 0b00000100U, 0b00100000U, 0b00000010U
    };

    // scalars used to add offset to the current position inside the octree
    // scalars used by scale to construct and offset
    static constexpr const i8 indexToPrefix[8][3] = {
            {-1, -1, -1}, {-1, -1, 1}, {-1, 1, -1}, {-1, 1,  1},
            { 1, -1, -1}, { 1, -1, 1}, { 1, 1, -1}, { 1, 1,  1}
    };

    /**
     * @brief Selecting a child from the current node
     *
     * @param packedVoxel The high 32 bit of the voxel containing the position
     * @param packedDataHighP The high 32 bit of the current node containing position and scale
     *
     * @return A u8 mask for the chosen child of the current node
     */

    static inline
    auto selectChild(u32 packedVoxel, u32 packedDataHighP) -> u8 {
        return (((packedVoxel & xAnd) >= (packedDataHighP & xAnd)) << 2) |
               (((packedVoxel & yAnd) >= (packedDataHighP & yAnd)) << 1) |
                ((packedVoxel & zAnd) >= (packedDataHighP & zAnd));
    }

    /**
     * @brief Building a cubic box enclosing a certain section of the octree
     *
     * @param childMask The targeted child of the curret node
     * @param packedData The high 32 bit of the current node containing position and scale
     *
     * @return A u32 mask equal to the packed data's high 32 bit of a node
     */

    static inline
    auto buildBbox(u8 childMask, u32 packedData) -> u32 {
        auto [pX, pY, pZ] = indexToPrefix[childMask];

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
     * @brief Searches for a specific voxel via its position
     *
     * @param position The voxel position compressed in a u16
     * @param current A pointer referring to the current node
     *
     * @return A std::optional containing either the voxel or none
     */

    inline
    auto findNode(u32 packedVoxel, node::Node *current) -> std::optional<node::Node *> {
        for(;;) {
            if (!(current->_packed >> 56)) {

                // spherical approximation of the position
                // via distance between the position and current node
                if (((current->_packed >> 50) & 0x3F)) {
                    auto posVec3 = glm::vec3 {
                            (packedVoxel >> 13) & 0x1F,
                            (packedVoxel >>  8) & 0x1F,
                            (packedVoxel >>  3) & 0x1F
                    };

                    auto rootVec3 = glm::vec3 {
                            (current->_packed >> 45) & 0x1F,
                            (current->_packed >> 40) & 0x1F,
                            (current->_packed >> 35) & 0x1F
                    };

                    u8 scale = 1 << ((current->_packed >> 32) & 0x7);
                    if ((std::pow(glm::distance(posVec3, rootVec3), 2) * 2) <= std::pow(scale, 2))
                        return std::make_optional(current);
                }
                else {
                    return std::nullopt;
                }
            }

            // if the segment is not in use the voxel doesn't exist
            auto index = selectChild(packedVoxel, current->_packed >> 32);
            if (!((current->_packed >> 56) & indexToSegment[index]))
                return std::nullopt;

            current = &current->_nodes[index];
        }
    }

    /**
     * @brief Inserts a specific voxel via its position
     *
     * Calculates the position of the voxel inside the tree via using the u32 higher half of
     * the last node's (or root's) _packed packed data.
     * The high 16 bit of the lower 32 bit of _packed will be set with the packedVoxel high 16 bit of
     * the lower 32 bit to contain the chunk information.
     * The lowest 16 bit of _packed will be set to 0 unless the node becomes a voxel.
     *
     * @param packedVoxel The voxel compressed in a u64
     * @param packedData The current bounding box of the last node (or root of the tree)
     *
     * @return The address of inserted Voxel
     */

    inline
    auto insertNode(u64 packedVoxel, u32 packedData, node::Node *current) -> node::Node * {
        for(;;) {
            if ((1 << (packedData & 0x7)) == BASE_SIZE) {

                // setting all faces to visible
                // shifting octree local data to the higher 32 bit
                // adding the voxel unique data
                current->_packed = (static_cast<u64>(0x3F) << 50) |
                                   (static_cast<u64>(packedData) << 32) |
                                   (packedVoxel & UINT32_MAX);
                return current;
            }
            else {
                u8 index    = selectChild(packedVoxel >> 32, packedData);
                u8 segment  = indexToSegment[index];
                u8 segments = current->_packed >> 56;

                // if the node does not contain any children
                if (!segments) {

                    // initializing the current node's packed data field with
                    // segments, faces, position and high 16 bit of the packedVoxel containing chunk data
                    // lowest 16 bit are set to 0
                    current->_packed = (static_cast<u64>(segment) << 56) |
                                       (static_cast<u64>(packedData) << 32) |
                                       (packedVoxel & 0xFFFF0000);


                    current->_nodes = util::tagged_ptr::make_tagged<node::Node [8], u16>();
                    ASSERT(current->_nodes.get<node::Node>());
                }

                // if the chosen segment is not in use
                if (!(segments & segment))
                    current->_packed |= (static_cast<u64>(segment) << 56) |
                                        (static_cast<u64>(0x3F) << 50);

                ASSERT(&current->_nodes[0])
                packedData = buildBbox(index, packedData);
                current    = &current->_nodes[index];
            }
        }
    }
}

#endif //OPENGL_3D_ENGINE_NODE_INLINE_H

