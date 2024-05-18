//
// Created by Luis Ruisinger on 17.02.24.
//

#include <immintrin.h>
#include <cmath>

#include "Octree.h"
#include "../Chunk/Chunk.h"

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

namespace Octree {
    constexpr const u32 xAnd = (0x1F << 13);
    constexpr const u32 yAnd = (0x1F <<  8);
    constexpr const u32 zAnd = (0x1F <<  3);

    static const u8 indexToSegment[8] = {
            0b10000000U, 0b00001000U, 0b00010000U, 0b00000001U,
            0b01000000U, 0b00000100U, 0b00100000U, 0b00000010U
    };

    static const i8 indexToPrefix[8][3] = {
            {-1, -1, -1}, {-1, -1, 1}, {-1, 1, -1}, {-1, 1,  1},
            { 1, -1, -1}, { 1, -1, 1}, { 1, 1, -1}, { 1, 1,  1}
    };

    //
    //
    //

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

        // branchless due to abusing boolesh terms evaluating either to 1 or 0
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

    template<typename T> requires derivedFromBoundingVolume<T>
    static inline
    auto findNode(u32 packedVoxel, Node<T> *current) -> std::optional<Node<T> *> {
        while (true) {
            if (!(current->_packed >> 56)) {

                // spherical approximation of the position
                // via distance between the position and current node
                if (((current->_packed >> 50) & 0x3F)) {
                    auto posVec3  = glm::vec3((packedVoxel >> 13) & 0x1F,
                                              (packedVoxel >>  8) & 0x1F,
                                              (packedVoxel >>  3) & 0x1F);

                    auto rootVec3 = glm::vec3((current->_packed >> 45) & 0x1F,
                                              (current->_packed >> 40) & 0x1F,
                                              (current->_packed >> 35) & 0x1F);

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

    template<typename T> requires derivedFromBoundingVolume<T>
    static inline
    auto insertNode(u64 packedVoxel, u32 packedData, Node<T> *current) -> Node<T> * {
        while (true) {
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

                if (!segments) {

                    // initializing the current node's packed data field with
                    // segments, faces, position and high 16 bit of the packedVoxel containing chunk data
                    // lowest 16 bit are set to 0
                    current->_packed = (static_cast<u64>(segment) << 56) |
                                       (static_cast<u64>(packedData) << 32) |
                                       (packedVoxel & 0xFFFF0000);

                    current->_nodes  = static_cast<Node<T> *>(
                            std::aligned_alloc(alignof(Node<T>), sizeof(Node<T>) * 8));
                }

                if (!(segments & segment)) {

                    // adding a new segment and initializing its corresponding node
                    current->_packed |= (static_cast<u64>(segment) << 56) | (static_cast<u64>(0x3F) << 50);
                    current->_nodes[index] = {};
                }

                packedData = buildBbox(index, packedData);
                current    = &current->_nodes[index];
            }
        }
    }

    //
    //
    //

    /**
     * @brief Default constructor for the Node class.
     *
     * Initializes the node with null child nodes and zeroed-out packed data.
     *
     */

    template<typename T> requires derivedFromBoundingVolume<T>
    Node<T>::Node()
        : _nodes{nullptr}
        , _packed{0}
    {}

    /**
     * @brief Destructor for the Node class.
     *
     * Frees memory allocated for child nodes if necessary.
     */

    template<typename T> requires derivedFromBoundingVolume<T>
    Node<T>::~Node() noexcept {
        if (!_nodes)
            return;

        u8 segments = (_packed >> 56) & 0xFF;
        if (segments) {
            for (u8 i = 0; i < 8; ++i) {
                if (segments & indexToSegment[i])
                    _nodes[i].~Node<T>();
            }

            std::free(_nodes);
        }
    }

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Node<T>::updateFaceMask(u16 packedChunk) -> u8 {
        if (!_nodes)
            return 0;

        u64 faces = 0;
        u8 segments = _packed >> 56;

        _packed |= packedChunk << 16;
        if (!segments)
            return (_packed >> 50) & 0x3F;

        for (u8 i = 0; i < 8; ++i) {
            if (segments & indexToSegment[i]) {
                faces |= _nodes[i].updateFaceMask(packedChunk);
            }
        }

        _packed |= faces << 50;
        return faces;
    }

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Node<T>::recombine(std::stack<Node *> &stack) -> void {
        u8 segments = _packed >> 56;
        
        if (!segments)
            return;

        stack.push(this);

        for (u8 i = 0; i < 8; ++i)
            if (segments & indexToSegment[i])
                _nodes[i].recombine(stack);

        Node<T> *current;

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

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Node<T>::cull(const Args<T> &args) const -> void {
        u64 faces = (_packed >> 50) & args._camera.getCameraMask();

        if (!faces)
            return;

        auto scale = 1 << ((_packed >> 32) & 0x7);
        auto position = glm::vec3((_packed >> 45) & 0x1F, (_packed >> 40) & 0x1F, (_packed >> 35) & 0x1F);

        if ((scale > 4) && !args._camera.inFrustum(args._point + position, scale))
            return;

        auto segments = _packed >> 56;
        if (segments) {
            for (u8 i = 0; i < 8; ++i) {
                if (segments & indexToSegment[i])
                    _nodes[i].cull(args);
            }
        }
        else if (faces)
            args._renderer.addVoxel(_packed & ((faces << 50) | 0x03FFFFFFFFFFFF));
    }

    //
    //
    //

    // ----------------------
    // Octree implementation

    template<typename T> requires derivedFromBoundingVolume<T>
    Octree<T>::Octree()
        : _root{std::make_unique<Node<T>>()}
    {}

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Octree<T>::addPoint(u64 packedVoxel) -> Node<T> * {
        return insertNode(packedVoxel, _packed, _root.get());
    }

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Octree<T>::removePoint(u16 position) -> void {}

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Octree<T>::cull(const vec3f &position,
                         const Camera::Perspective::Camera &camera,
                         const Renderer::Renderer &renderer) const -> void {
        const Args<T> args = {position, camera, renderer};
        _root->cull(args);
    }

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Octree<T>::find(u32 packedVoxel) const -> std::optional<Node<T> *> {
        return findNode(packedVoxel, _root.get());
    }

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Octree<T>::updateFaceMask(u16 mask) -> u8 {
        return _root->updateFaceMask(mask);
    }

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Octree<T>::recombine() -> void {
        std::stack<Node<T> *> stack;
        _root->recombine(stack);
    }

    template class Octree<BoundingVolume>;
    template class Node<BoundingVolume>;
}