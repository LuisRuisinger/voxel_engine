//
// Created by Luis Ruisinger on 26.05.24.
//

#include <immintrin.h>
#include <cmath>

#include "node_inline.h"
#include "../presenter.h"

#define ONE_BIT_SET_ONLY(_v)                       ((_v) && !((_v) & ((_v) - 1)))
#define SET_BIT_INDEX(_v)                          ((__builtin_ffs(_v)) - 1)
#define CHECK_BIT(_v, _p)                          ((_v) & (1 << (_p)))
#define EXTRACT_ID(_v)                             ((_v) & 0x3FF)
#define SOME_BIT_DIFFERS(_p, _i1, _i2, _i3, _i4, _bit)                                                                   \
    (((((_p)->_nodes[(_i1)]._packed >> 50) & 0x3F) ^ (((_p)->_nodes[(_i2)]._packed >> 50) & 0x3F)) & (_bit) || \
     ((((_p)->_nodes[(_i1)]._packed >> 50) & 0x3F) ^ (((_p)->_nodes[(_i3)]._packed >> 50) & 0x3F)) & (_bit) || \
     ((((_p)->_nodes[(_i1)]._packed >> 50) & 0x3F) ^ (((_p)->_nodes[(_i4)]._packed >> 50) & 0x3F)) & (_bit))

namespace core::level::node {

    // TODO: why the fuck are we actually fetching recursively the face mask ?? For what reason ??
    /**
     * @brief  Recursively traverses octree to update the nodes' packed_data with new chunk position.
     * @param  packed_chunk Bitmask containing the new chunk's position.
     * @return ????????
     */
    auto Node::updateFaceMask(u16 packed_chunk) -> u8 {
        if (this->nodes.is_null())
            return 0;

        u64 faces = 0;
        u8 segments = this->packed_data >> 56;

        this->packed_data &= static_cast<u64>(UINT32_MAX) << 32 | static_cast<u64>(UINT16_MAX);
        this->packed_data |= static_cast<u64>(packed_chunk) << 16;

        if (!segments)
            return static_cast<u8>((this->packed_data >> 50) & 0x3F);

        for (u8 i = 0; i < 8; ++i) {
            if (segments & node_inline::index_to_segment[i]) {
                faces |= static_cast<u64>(this->nodes[i].updateFaceMask(packed_chunk));
            }
        }

        this->packed_data |= faces << 50;
        return static_cast<u8>(this->packed_data >> 50) & 0x3F;
    }

    auto Node::recombine(std::stack<Node *> &stack) -> void {
        /*
        u8 segments = this->packed_data >> 56;

        if (!segments)
            return;

        stack.push(this);

        for (u8 i = 0; i < 8; ++i)
            if (segments & node_inline::index_to_segment[i])
                this->nodes[i].recombine(stack);

        Node *current;

        // combining same sized volumes
        while (!stack.empty()) {
            current = stack.top();

            if ((current->packed_data >> 56) ^ 0xFF)
                return;

            u8 faces   = 0;
            u8 voxelID = 0;

            for (u8 i = 0; i < 8; ++i) {

                // the voxelID's need to match, also the voxelID cannot be 0 which would indicate
                // that the node does not contain a valid voxel
                if ( (current->nodes[i].packed_data >> 56) ||
                     (current->nodes[i].packed_data & 0xFF) != (current->nodes[0].packed_data & 0xFF) ||
                     !(current->nodes[i].packed_data & 0xFF))
                    return;

                // determine which faces to cull
                faces |= (current->nodes[i].packed_data >> 50) & 0x3F;
            }

            voxelID = current->nodes[0].packed_data & 0xFF;

            // check if faces would be constructed that are partially occluded
            if (SOME_BIT_DIFFERS(2, 3, 6, 7, TOP_BIT)   || SOME_BIT_DIFFERS(0, 2, 4, 6, BACK_BIT) ||
                SOME_BIT_DIFFERS(1, 3, 5, 7, FRONT_BIT) || SOME_BIT_DIFFERS(0, 1, 2, 3, LEFT_BIT) ||
                SOME_BIT_DIFFERS(4, 5, 6, 7, RIGHT_BIT) || SOME_BIT_DIFFERS(0, 1, 4, 5, BOTTOM_BIT))
                return;

            std::free(current->nodes);

            // deleting the highest 14 bit, indicating a voxel or leaf node
            current->packed_data &= ~(static_cast<u64>(0x3FFF) << 50);
            current->packed_data |= static_cast<u64>(voxelID) << 50;

            // adding the voxelID to the node
            current->packed_data |= voxelID;

            stack.pop();
        }
         */
    }

    auto Node::face_merge() -> u8 {
        if (this->nodes.is_null())
            return 0;
        // TODO: check if set of segments exists
        // TODO: check if all children that form a face are equal
        // TODO: if contain children call this recursivly on children
        // TODO: mask bitmask
        // TODO: set own TaggedPtr to the value and return the value
        u8 face_merge = 0;
        return face_merge;
    }

    const constexpr u64 vertex_clear_mask = 0x0003FFFFFFFF00FFU;

    auto Node::cull(const Args &args, camera::culling::CollisionType type) const -> void {
        const u64 faces = this->packed_data & (static_cast<u64>(args._camera.getCameraMask()) << 50);

        if (!faces)
            return;

        if ((type == camera::culling::INTERSECT) &&
            (this->packed_data & node_inline::exponent_and) > node_inline::exponent_check) {
            auto scale = 1 << ((this->packed_data >> 32) & 0x7);
            auto position = glm::vec3 {
                    (this->packed_data >> 45) & 0x1F, (this->packed_data >> 40) & 0x1F, (this->packed_data >> 35) & 0x1F
            };

            type = args._camera.inFrustum_type(args._point + position, scale);
            if (type == camera::culling::CollisionType::OUTSIDE)
                return;
        }

        auto segments = this->packed_data >> 56;
        if (segments) {
            for (u8 i = 0; i < 8; ++i) {
                if (segments & node_inline::index_to_segment[i])
                    this->nodes[i].cull(args, type);
            }
        }
        else if (faces) {
            auto &ref = args._platform.get_presenter().get_structure(0).mesh();

#ifdef __AVX2__
            __m256i voxelVec = _mm256_set1_epi64x(this->packed_data & vertex_clear_mask);

            for (size_t i = 0; i < 6; ++i) {
                if ((faces >> 50) & (1 << i)) [[unlikely]] {
                    __m256i vertexVec = _mm256_loadu_si256(reinterpret_cast<__m256i const *>(ref[i].data()));
                    args._voxelVec.emplace_back(_mm256_or_si256(vertexVec, voxelVec));
                }
            }
#else
            packedVoxel &= mask;

            for (size_t i = 0; i < 6; ++i) {
                if (faces & (1 << i)) [[unlikely]] {
                    auto &face = ref[i];

                    for (auto vertex : face)
                        args._voxelVec.emplace_back(vertex | packedVoxel);
                }
            }
#endif
        }
    }
}