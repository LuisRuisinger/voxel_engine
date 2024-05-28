//
// Created by Luis Ruisinger on 26.05.24.
//

#include <immintrin.h>
#include <cmath>

#include "node_inline.h"

#define ONE_BIT_SET_ONLY(_v)                       ((_v) && !((_v) & ((_v) - 1)))
#define SET_BIT_INDEX(_v)                          ((__builtin_ffs(_v)) - 1)
#define CHECK_BIT(_v, _p)                          ((_v) & (1 << (_p)))
#define EXTRACT_ID(_v)                             ((_v) & 0x3FF)
#define SOME_BIT_DIFFERS(_i1, _i2, _i3, _i4, _bit) ((((current->_nodes[(_i1)]._packed >> 50) & 0x3F) ^            \
                                                     ((current->_nodes[(_i2)]._packed >> 50) & 0x3F)) & (_bit) || \
                                                    (((current->_nodes[(_i1)]._packed >> 50) & 0x3F) ^            \
                                                     ((current->_nodes[(_i3)]._packed >> 50) & 0x3F)) & (_bit) || \
                                                    (((current->_nodes[(_i1)]._packed >> 50) & 0x3F) ^            \
                                                     ((current->_nodes[(_i4)]._packed >> 50) & 0x3F)) & (_bit))

namespace core::level::node {

    /**
     * @brief Default constructor for the Node class.
     *
     * Initializes the node with null child nodes and zeroed-out packed data.
     *
     */

    Node::Node()
            : _nodes{nullptr}
            , _packed{0}
    {}

    /**
     * @brief Destructor for the Node class.
     *
     * Frees memory allocated for child nodes if necessary.
     */

    Node::~Node() noexcept {
        if (!_nodes)
            return;

        u8 segments = (_packed >> 56) & 0xFF;
        if (segments) {
            for (u8 i = 0; i < 8; ++i) {
                if (segments & node_inline::indexToSegment[i])
                    _nodes[i].~Node();
            }

            std::free(_nodes);
        }
    }

    auto Node::updateFaceMask(u16 packedChunk) -> u8 {
        if (!_nodes)
            return 0;

        u64 faces = 0;
        u8 segments = _packed >> 56;

        _packed |= packedChunk << 16;
        if (!segments)
            return (_packed >> 50) & 0x3F;

        for (u8 i = 0; i < 8; ++i) {
            if (segments & node_inline::indexToSegment[i]) {
                faces |= _nodes[i].updateFaceMask(packedChunk);
            }
        }

        _packed |= faces << 50;
        return static_cast<u8>(_packed >> 50) & 0x3F;
    }

    auto Node::recombine(std::stack<Node *> &stack) -> void {
        u8 segments = _packed >> 56;

        if (!segments)
            return;

        stack.push(this);

        for (u8 i = 0; i < 8; ++i)
            if (segments & node_inline::indexToSegment[i])
                _nodes[i].recombine(stack);

        Node *current;

        // combining same sized volumes
        while (!stack.empty()) {
            current = stack.top();

            if ((current->_packed >> 56) ^ 0xFF)
                return;

            u8 faces   = 0;
            u8 voxelID = 0;

            for (u8 i = 0; i < 8; ++i) {

                // the voxelID's need to match, also the voxelID cannot be 0 which would indicate
                // that the node does not contain a valid voxel
                if ( (current->_nodes[i]._packed >> 56) ||
                     (current->_nodes[i]._packed & 0xFF) != (current->_nodes[0]._packed & 0xFF) ||
                     !(current->_nodes[i]._packed & 0xFF))
                    return;

                // determine which faces to cull
                faces |= (current->_nodes[i]._packed >> 50) & 0x3F;
            }

            voxelID = current->_nodes[0]._packed & 0xFF;

            // check if faces would be constructed that are partially occluded
            if (SOME_BIT_DIFFERS(2, 3, 6, 7, TOP_BIT)   || SOME_BIT_DIFFERS(0, 2, 4, 6, BACK_BIT) ||
                SOME_BIT_DIFFERS(1, 3, 5, 7, FRONT_BIT) || SOME_BIT_DIFFERS(0, 1, 2, 3, LEFT_BIT) ||
                SOME_BIT_DIFFERS(4, 5, 6, 7, RIGHT_BIT) || SOME_BIT_DIFFERS(0, 1, 4, 5, BOTTOM_BIT))
                return;

            std::free(current->_nodes);

            // deleting the highest 14 bit, indicating a voxel or leaf node
            current->_packed &= ~(static_cast<u64>(0x3FFF) << 50);
            current->_packed |= static_cast<u64>(voxelID) << 50;

            // adding the voxelID to the node
            current->_packed |= voxelID;

            stack.pop();
        }
    }

    const constexpr u64 vertex_clear_mask = 0x0003FFFFFFFF00FFU;

    auto Node::cull(const Args &args, camera::culling::CollisionType type) const -> void {
        const u64 faces = _packed & (static_cast<u64>(args._camera.getCameraMask()) << 50);

        if (!faces)
            return;

        if (type == camera::culling::INTERSECT)
        if ((_packed & node_inline::exponent_and) > node_inline::exponent_check) {
            auto scale = 1 << ((_packed >> 32) & 0x7);
            auto position = glm::vec3((_packed >> 45) & 0x1F, (_packed >> 40) & 0x1F, (_packed >> 35) & 0x1F);

            type = args._camera.inFrustum_type(args._point + position, scale);
            if (type == camera::culling::CollisionType::OUTSIDE)
                return;
        }

        auto segments = _packed >> 56;
        if (segments) {
            for (u8 i = 0; i < 8; ++i) {
                if (segments & node_inline::indexToSegment[i])
                    _nodes[i].cull(args, type);
            }
        }
        else if (faces) {
            auto &ref = args._renderer._structures[0].mesh();

#ifdef __AVX2__
            __m256i voxelVec = _mm256_set1_epi64x(_packed & vertex_clear_mask);

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
                        args._voxelVec->emplace_back(vertex | packedVoxel);
                }
            }
#endif
        }
    }
}