//
// Created by Luis Ruisinger on 26.05.24.
//

#ifdef __AVX2__
#include <immintrin.h>
#define GLM_FORCE_AVX
#endif

#include <cmath>

#include "node_inline.h"
#include "../chunk/chunk_renderer.h"
#include "../core/level/model/voxel.h"

namespace core::level::node {

    /** @brief Mask to extract packed data without the packed chunk information. */
    static constexpr const u64 mask_off_chunk =
            (UINT64_MAX << SHIFT_HIGH) | static_cast<u64>(UINT16_MAX);

    /** @brief Mask to transform a vertex point to a face. */
    static constexpr const u64 vertex_clear_mask = 0x0003FFFFFFFF00FFU;

    /** @brief Object containing a compressed representation of the sides of a voxel. */
    //static const model::voxel::CubeStructure cube_structure = {};

    /**
     * @brief  Updates the mask inside the packed voxel data and recursivly builds a face mask.
     * @param  mask Packed chunk data containing chunk offset and chunksegment offset.
     * @return Face mask for the current cubic area.
     */
    auto Node::update_face_mask(u16 mask) -> u8 {
        u8 faces = 0;
        u8 segments = this->packed_data >> 56;

        this->packed_data = (this->packed_data & mask_off_chunk) | (static_cast<u64>(mask) << 16);
        if (!segments)
            return static_cast<u8>((this->packed_data >> 50) & MASK_6);

        for (u8 i = 0; i < 8; ++i)
            if (segments & (1 << i))
                faces |= this->nodes->operator[](i).update_face_mask(mask);

        this->packed_data |= static_cast<u64>(faces) << 50;
        return faces;
    }

    /**
     * @brief Updates the mask inside the packed voxel data.
     *        Happens when chunks are moved in another loaded area.
     * @param mask Packed chunk data containing chunk offset and chunksegment offset.
     */
    auto Node::update_chunk_mask(u16 mask) -> void {
        this->packed_data = (this->packed_data & mask_off_chunk) | (static_cast<u64>(mask) << 16);

        u8 segments = this->packed_data >> 56;
        for (u8 i = 0; i < 8; ++i)
            if (segments & (1 << i))
                this->nodes->operator[](i).update_chunk_mask(mask);
    }

    /**
     * @brief Recombines the underlying chunk_data_structure to a SVO.
     *        Cubic areas of equal voxels will be combined to a bigger voxel.
     *        Therefore we would traverse less nodes and can destroy children
     *        (which represent smaller areas / voxels).
     * @param stack
     */
    auto Node::recombine() -> void {
        u8 segments = this->packed_data >> 56;

        // removing unnecessary nodes containing <= 1 child
        // traversal path to leaves will be shorter
        // and we reduce the amount of memory to store the platform
        if (!(segments & (segments - 1))) {
            for (u8 i = 0; i < 8; ++i)
                if (segments & (1 << i)) {
                    this->packed_data = this->nodes->operator[](i).packed_data;
                    this->nodes = std::move(this->nodes->operator[](i).nodes);

                    segments = this->packed_data >> 56;
                    break;
                }
        }

        // recombine children
        for (u8 i = 0; i < 8; ++i)
            if (segments & (1 << i))
                this->nodes->operator[](i).recombine();

        // checks if all children equal each other
        if (!node_inline::check_combinable(this))
            return;

        // deleting highest 14 bit and lowest 9 bit
        // deletes segments indicating no sub areas follow
        // deletes dirty faces the recalculate them in an higher order volume
        // deletes dirty voxel_ID to assign one of the subareas
        this->packed_data &= (UINT64_MAX >> 14) & (UINT64_MAX << 9);
        this->packed_data |= node_inline::combine_faces(this);
        this->packed_data |= this->nodes->operator[](0).packed_data & MASK_VOXEL_ID;

        // reset nodes
        this->nodes = {};
    }

    /**
     * @brief Builds mesh through extraction of visible faces.
     * @param args
     * @param type Frustum intersection type used to determine
     *             if we need to test against the frustum.
     */
    auto Node::cull(Args &args, util::culling::CollisionType type) const -> void {
        const u64 faces = (this->packed_data >> 50) & static_cast<u64>(args._camera.get_mask());
        if (!faces)
            return;

        // frustum check
        if (((this->packed_data & node_inline::exponent_and) > node_inline::exponent_check) &&
            (type == util::culling::INTERSECT)) {
            const auto scale = 1 << ((this->packed_data >> SHIFT_HIGH) & MASK_3);
            auto position = glm::vec3 {
                    (this->packed_data >> 45) & MASK_5,
                    (this->packed_data >> 40) & MASK_5,
                    (this->packed_data >> 35) & MASK_5
            };
            position += glm::vec3(args._point);

            if ((type = args._camera.frustum_collision(position, scale)) ==
                util::culling::CollisionType::OUTSIDE) {
                return;
            }
        }

        auto segments = this->packed_data >> 56;
        if (segments) {
            for (u8 i = 0; i < 8; ++i) {
                if (segments & (1 << i)) {
                    this->nodes->operator[](i).cull(args, type);
                }
            }
        }
        else {
            ASSERT_EQ(faces);

            #ifdef __AVX2__
            __m256i voxelVec = _mm256_set1_epi64x(this->packed_data & vertex_clear_mask);

            for (size_t i = 0; i < 6; ++i) {

                // we often only see 1 face
                if (faces & (1 << i)) [[unlikely]] {
                    __m256i vertexVec = _mm256_or_si256(model::voxel::cube_structure.mesh()[i], voxelVec);
                    _mm256_store_si256(
                            const_cast<__m256i *>(&args._voxelVec[args.actual_size]),
                            vertexVec);

                    ++args.actual_size;
                }
            }

            #else
            packedVoxel &= mask;

            for (size_t i = 0; i < 6; ++i) {
                if (faces & (1 << i)) [[unlikely]] {
                    auto &face = cube_structure.mesh()[i];

                    for (auto vertex : face)
                        args._voxelVec.emplace_back(vertex | packedVoxel);
                }
            }
            #endif
        }
    }

    /**
     * @brief  Masks the leafs and increments a counter if mask contains bitmask.
     * @param  mask The bitmask to test leaves against.
     * @return Counter how many leaves contain the bitmask.
     */
    auto Node::count_mask(u64 mask) -> size_t {
        auto sum = 0;
        auto segments = this->packed_data >> 56;

        if (segments) {
            for (auto i = 0; i < 8; ++i)
                if (segments & (1 << i))
                    sum += this->nodes->operator[](i).count_mask(mask);
        }
        else {
            for (auto i = 0; i < 6; ++i)
                if (this->packed_data & mask)
                    ++sum;
        }

        return sum;
    }

    /**
     * @brief  Searches for a node using a ray-AABB intersection function
     *         taking in the position of a node and the scale of the node's AABB.
     * @param  chunk_pos The position of the chunk containing this tree relative to the
     *                   current world root.
     * @param  fun       Arbitrary function taking in the position and scale of the
     *                   current Node. Returns a float scalar indicating how an orientation
     *                   would need to be scaled to hit the AABB of the node.
     * @return The shortest found scalar.
     */
    auto Node::find_node(
            const glm::vec3 &chunk_pos,
            std::function<f32(const glm::vec3 &, const u32)> &fun) -> f32 {
        if (!(this->packed_data >> 50 & MASK_6))
            return std::numeric_limits<f32>::max();

        const auto scale = 1 << ((this->packed_data >> SHIFT_HIGH) & MASK_3);
        const auto position = chunk_pos + glm::vec3 {
                (this->packed_data >> 45) & MASK_5,
                (this->packed_data >> 40) & MASK_5,
                (this->packed_data >> 35) & MASK_5
        };

        auto ray_scale = fun(position, scale);
        auto segments = this->packed_data >> 56;

        if (segments) {
            ray_scale = std::numeric_limits<f32>::max();
            for (auto i = 0; i < 8; ++i)
                if (segments & (1 << i)) {
                    auto ret = this->nodes->operator[](i).find_node(chunk_pos, fun);
                    ray_scale = ret < ray_scale ? ret : ray_scale;
                }
        }

        return ray_scale;
    }
}