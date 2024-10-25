//
// Created by Luis Ruisinger on 26.05.24.
//

#ifndef OPENGL_3D_ENGINE_NODE_INLINE_H
#define OPENGL_3D_ENGINE_NODE_INLINE_H

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <memory>
#include <array>
#include <vector>
#include <stack>
#include <functional>

#ifdef __AVX2__
#include <immintrin.h>
#define GLM_FORCE_AVX
#endif

#include "node.h"

#include "../util/assert.h"
#include "../util/defines.h"
#include "../util/aabb.h"

#define BASE_SIZE     0x1
#define MASK_3        0x7
#define MASK_5        0x1F
#define MASK_6        0x3F
#define SHIFT_HIGH    0x20
#define MASK_VOXEL_ID 0x1FF

namespace core::level::node_inline {

    /** @brief Mask to extract the x-offset inside the chunk from packed_data */
    static constexpr const u32 x_and = 0x1F << 0xD;

    /** @brief Mask to extract the y-offset inside the chunk from packed_data */
    static constexpr const u32 y_and = 0x1F << 0x8;

    /** @brief Mask to extract the z-offset inside the chunk from packed_data */
    static constexpr const u32 z_and = 0x1F << 0x3;

    /** @brief Extract the exponent of the scale from packed_data */
    static constexpr const u64 exponent_and = 0x700000000;

    static constexpr const u64 mask_coords = (0x1 << 0x12) - 0x1;

    /** @brief Mask packed_data with exponent threshold */
    static constexpr const u64 exponent_check = static_cast<u64>(0x2) << SHIFT_HIGH;

    /** @brief Extract everything of packed_data except the highest 14 bit (segment, faces) */
    static constexpr const u64 save_and = 0x03FFFFFFFFFFFF;

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
    inline static
    auto select_child(u32 packed_voxel, u32 packed_data_high) -> u8 {
#ifdef __AVX512VL__
        const __m128i _mask = _mm_set_epi32(0x0, x_and, y_and, z_and);

        __m128i _pv = _mm_set1_epi32(packed_voxel);
        __m128i _pd = _mm_set1_epi32(packed_data_high);

        _pv = _mm_and_si128(_pv, _mask);
        _pd = _mm_and_si128(_pd, _mask);

        // (a >= b) direct computable and storable in u8 mask
        return _mm_cmpge_epi32_mask(_pv, _pd);

#elif __AVX2__
        const __m256i _and = _mm256_set1_epi32(0x1);
        const __m256i _mask = _mm256_set_epi32(0x0, 0x0, 0x0, 0x0, 0x0, x_and, y_and, z_and);

        __m256i _pv = _mm256_set1_epi32(packed_voxel);
        __m256i _pd = _mm256_set1_epi32(packed_data_high);

        _pv = _mm256_and_si256(_pv, _mask);
        _pd = _mm256_and_si256(_pd, _mask);

        // (a >= b) is equal to !(b > a)
        _pd = _mm256_cmpgt_epi32(_pd, _pv);

        // !(b > a)
        _pd = _mm256_xor_si256(_pd, _and);
        _pd = _mm256_and_si256(_pd, _and);

        return (_mm256_extract_epi32(_pd, 2) << 0x2) |
               (_mm256_extract_epi32(_pd, 1) << 0x1) |
               (_mm256_extract_epi32(_pd, 0));
#else
        return (((packed_voxel & x_and) >= (packed_data_high & x_and)) << 0x2) |
               (((packed_voxel & y_and) >= (packed_data_high & y_and)) << 0x1) |
                ((packed_voxel & z_and) >= (packed_data_high & z_and));
#endif
    }

    /**
     * @brief  Building a cubic box enclosing a certain section of the chunk_data_structure
     * @param  child_mask  The targeted child of the curret node
     * @param  packed_data_high32 The high 32 bit of the current node containing position and scale
     * @return A u32 mask equal to the packed data's high 32 bit of a node
     */
    inline static
    auto build_AABB(u8 child_mask, u32 packed_data_high32) -> u32 {
        auto [prefix_x, prefix_y, prefix_z] = index_to_prefix[child_mask];

        // calculating 2^(n + 1) / 2^2 = 2^n / 2 = 2^(n - 1)
        // n + 1 because we need to fake a 32^3 segment as 64^3 to never need floats
        u8 scale  = (1 << (packed_data_high32 & MASK_3)) >> 0x1;

        u8 x = ((packed_data_high32 >> 0xD) & MASK_5) << 0x1;
        u8 y = ((packed_data_high32 >> 0x8) & MASK_5) << 0x1;
        u8 z = ((packed_data_high32 >> 0x3) & MASK_5) << 0x1;

        x += scale * prefix_x;
        y += scale * prefix_y;
        z += scale * prefix_z;

        // the new packed_data_high32 will always have its packed segments set to 0
        // selecting a node will set the respective segment
        return (packed_data_high32 & (MASK_6 << 0x12)) |

               // setting current position
               // because we shiftet the coordinates to the left
               // we can just extract the higher number and shift it less
               ((x & 0x3E) << 0xC) |
               ((y & 0x3E) << 0x7) |
               ((z & 0x3E) << 0x2) |

               // the new factor
               ((packed_data_high32 & MASK_3) - 0x1);
    }

    /**
     * @brief  Searches for a specific voxel via its position
     * @param  position The voxel position compressed in a u16
     * @param  current  A pointer referring to the current node
     * @return A std::optional containing either the voxel or none
     */
    inline static
    auto find_node(u32 packed_data_high32, node::Node *current) -> node::Node * {
        const auto position_vec = glm::vec3 {
                (packed_data_high32 >> 0xD) & MASK_5,
                (packed_data_high32 >> 0x8) & MASK_5,
                (packed_data_high32 >> 0x3) & MASK_5
        };

        const auto aabb = util::aabb::AABB<f32>().translate(position_vec);

        for(;;) {
            if (!(current->packed_data >> 0x38)) {

                // node default init without any content
                if (!((current->packed_data >> 0x32) & MASK_6))
                    return nullptr;

                const auto root_vec = glm::vec3 {
                        (current->packed_data >> 0x2D) & MASK_5,
                        (current->packed_data >> 0x28) & MASK_5,
                        (current->packed_data >> 0x23) & MASK_5
                };

                const u8 scale = 1 << ((current->packed_data >> SHIFT_HIGH) & MASK_3);

                // both aabb's are BASE_SIZE, thus they can be compared by just the position
                // there is no possible other voxel which could fit if the positions
                // do not equal each other
                if (scale == BASE_SIZE) {
                    return root_vec == position_vec ? current : nullptr;
                }

                // aabb - aabb intersection
                auto intersect = util::aabb::AABB<f32>()
                        .translate(root_vec)
                        .scale_center(static_cast<f32>(scale))
                        .translate(0.5F)
                        .intersection(aabb);

                if (intersect)
                    return current;
            }

            // if the segment is not in use the voxel doesn't exist
            const auto index = select_child(packed_data_high32, current->packed_data >> SHIFT_HIGH);
            if (!((current->packed_data >> 0x38) & (0x1 << index)))
                return nullptr;

            current = &(current->nodes->operator[](index));
            ASSERT_EQ(current);
        }
    }

    /**
     * @brief Inserts a specific voxel via its position
     *
     * Calculates the position of the voxel inside the tree via using the u32 higher half of
     * the last node's (or root's) packed_data packed data.
     * The high 16 bit of the lower 32 bit of packed_data will be set
     * with the packed_voxel high 16 bit of the lower 32 bit to contain the chunk information.
     * The lowest 16 bit of packed_data will be set to 0 unless the node becomes a voxel.
     *
     * @param  packed_voxel The voxel compressed in a u64
     * @param  data         The current bounding box of the last node (or root of the tree)
     * @return The address of inserted Voxel
     */
    inline static
    auto insert_node(u64 packed_voxel, u32 data, node::Node *current) -> node::Node * {
        for(;;) {
            ASSERT_EQ(current);
            if ((0x1 << (data & MASK_3)) == BASE_SIZE) {

                // setting all faces to visible
                // shifting chunk_data_structure local data to the higher 32 bit
                // adding the voxel unique data
                current->packed_data =
                        (static_cast<u64>(MASK_6) << 0x32) |
                        (static_cast<u64>(data) << SHIFT_HIGH) |
                        (packed_voxel & UINT32_MAX);
                return current;
            }
            else {
                u8 index    = select_child(packed_voxel >> SHIFT_HIGH, data);
                u8 segment  = 1 << index;
                u8 segments = current->packed_data >> 0x38;

                // if the node does not contain any children
                if (!segments) {
                    // the following MUST happen in this order because this
                    // isn't protected for a multithreaded context
                    // if we would set the segment first and then initialize the nodes
                    // the behavior is undefined because another thread could search for
                    // this node and try to access it because its getting indicated by
                    // the bitmask before even the construction happened

                    // initializes the 8 new children with the default initializer of Node
                    current->nodes = std::make_unique<std::array<node::Node, 8>>();

                    // initializing the current node's packed data field with
                    // segments, faces, position and high 16 bit of the packed_voxel
                    // containing chunk data lowest 16 bit are set to 0
                    current->packed_data =
                            (static_cast<u64>(segment) << 0x38) |
                            (static_cast<u64>(data) << SHIFT_HIGH) |
                            (packed_voxel & 0xFFFF0000);
                }

                // if the chosen segment is not in use
                if (!(segments & segment))
                    current->packed_data |=
                            (static_cast<u64>(segment) << 0x38) |
                            (static_cast<u64>(MASK_6) << 0x32);

                ASSERT_EQ(current->nodes.get());
                data = build_AABB(index, data);
                current = &(current->nodes->operator[](index));
            }
        }
    }

    /**
     * @brief  Checks if all subareas of the current volume can be combined to the current volume.
     * @param  node Node ptr to the current node whose children we observe.
     * @return Boolean indicating if children are combinable
     *         to volume of the size of the current node.
     */
    template <typename ...Args>
    requires std::conjunction_v<std::is_integral<Args> ...>
    inline static
    auto check_combinable(node::Node *node) -> bool {

        // all children must be in use
        if ((node->packed_data >> 0x38) ^ 0xFF)
            return false;

#ifdef __AVX512VL__
        __m256i _pd = _mm256_set_epi32(
                node->nodes->operator[](0).packed_data,
                node->nodes->operator[](1).packed_data,
                node->nodes->operator[](2).packed_data,
                node->nodes->operator[](3).packed_data,
                node->nodes->operator[](4).packed_data,
                node->nodes->operator[](5).packed_data,
                node->nodes->operator[](6).packed_data,
                node->nodes->operator[](7).packed_data);

        __m256i _leaves = _mm256_srli_epi32(_pd, 0x38);
        if (_mm256_cmpge_epi32_mask(_leaves, _mm256_set1_epi32(0x0)))
            return false;

        _leaves = _mm256_srli_epi32(_pd, SHIFT_HIGH);
        _leaves = _mm256_and_si256(_leaves, _mm256_set1_epi32(MASK_3));
        if (_mm256_cmpneq_epi32_mask(
                _leaves,
                _mm256_set1_epi32(_mm256_extract_epi32(_leaves, 0)))) {
            return false;
        }

        _leaves = _mm256_and_si256(_pd, _mm256_set1_epi32(MASK_VOXEL_ID));
        if (_mm256_cmpneq_epi32_mask(
                _leaves,
                _mm256_set1_epi32(_mm256_extract_epi32(_leaves, 0)))) {
            return false;
        }
#else
        for (const auto &child : *node->nodes) {
            if (child.packed_data >> 0x38)
                return false;
        }

        const auto &first = (*node->nodes)[0];
        for (const auto &child : *node->nodes) {
            if (((child.packed_data >> SHIFT_HIGH) & MASK_3) !=
                ((first.packed_data >> SHIFT_HIGH) & MASK_3)) {
                return false;
            }
        }

        for (const auto &child : *node->nodes) {
            if ((child.packed_data & MASK_VOXEL_ID) !=
                (first.packed_data & MASK_VOXEL_ID)) {
                return false;
            }
        }

        return true;
#endif
    }

    /**
     * @brief  Combines faces of all children to a combined face mask.
     * @param  node Node ptr to the current node whose children we observe.
     * @return Non-shifted, combined face mask.
     */
    inline static
    auto combine_faces(node::Node *node) -> u64 {

        // mask to extract the faces of each node
        // packed node data in 64 bit representation assumed here
        static constexpr const u64 mask = static_cast<u64>(MASK_6) << 0x32;

#ifdef __AVX512VL__

        // experimental
        // TODO: maybe this is also usable for check_combinable instead of this many loads
        __m512 _low  = _mm512_load_epi64(reinterpret_cast<u64 *>(node->nodes.get()));
        __m512 _high = _mm512_load_epi64(reinterpret_cast<u64 *>(node->nodes.get() + 4));
#else
        u64 sum = 0;
        for (const auto &child : *node->nodes)
            sum |= child.packed_data;

        return sum & mask;
#endif
    }
}

#endif //OPENGL_3D_ENGINE_NODE_INLINE_H

