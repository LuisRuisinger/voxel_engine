//
// Created by Luis Ruisinger on 17.02.24.
//

#include <immintrin.h>
#include <cmath>

#include "Octree.h"
#include "../Chunk/Chunk.h"

#define NEIGHBOR_CHUNK(_cp, _op) { \
    _cp.x + (_op.x < 0 ? -CHUNK_SIZE : (_op.x > CHUNK_SIZE ? CHUNK_SIZE : 0)), \
    _cp.z + (_op.z < 0 ? -CHUNK_SIZE : (_op.z > CHUNK_SIZE ? CHUNK_SIZE : 0))  \
}

#define NEIGHBOR_CHUNK_COORD(_np) { \
(_np.x > CHUNK_SIZE ? _np.x - CHUNK_SIZE : (_np.x < 0 ? _np.x + CHUNK_SIZE : _np.x)),\
(_np.y > CHUNK_SIZE ? _np.y - CHUNK_SIZE : (_np.y < 0 ? _np.y + CHUNK_SIZE : _np.y)),\
(_np.z > CHUNK_SIZE ? _np.z - CHUNK_SIZE : (_np.z < 0 ? _np.z + CHUNK_SIZE : _np.z)) \
}

#define NEIGHBOR_SEGMENT(_cp, _op) { \
_cp.x,                               \
_cp.y + (_op.y < 0 ? -CHUNK_SIZE : (_op.y > CHUNK_SIZE ? CHUNK_SIZE : 0)), \
_cp.z                                \
}

#define NEIGHBOR_CHUNK_SEGMENT_COORD(_np) { \
_np.x,                                      \
(_np.y > CHUNK_SIZE ? _np.y - CHUNK_SIZE : (_np.y < 0 ? _np.y + CHUNK_SIZE : _np.y)),\
_np.z,                                      \
}

#define ONE_BIT_SET_ONLY(_v)                       ((_v) && !((_v) & ((_v) - 1)))
#define SET_BIT_INDEX(_v)                          ((__builtin_ffs(_v)) - 1)
#define CHECK_BIT(_v, _p)                          ((_v) & (1 << (_p)))
#define EXTRACT_ID(_v)                             ((_v) & 0x3FF)
#define SOME_BIT_DIFFERS(_i1, _i2, _i3, _i4, _bit) ((current->_nodes[(_i1)]._leaf->_voxelID ^ current->_nodes[(_i2)]._leaf->_voxelID) & (_bit) || \
                                                    (current->_nodes[(_i1)]._leaf->_voxelID ^ current->_nodes[(_i3)]._leaf->_voxelID) & (_bit) || \
                                                    (current->_nodes[(_i1)]._leaf->_voxelID ^ current->_nodes[(_i4)]._leaf->_voxelID) & (_bit))

namespace Octree {

    //
    //
    //

    // ----------------------
    // Octree implementation

    template<typename T> requires derivedFromBoundingVolume<T>
    Octree<T>::Octree(vec3f position)
        : _boundingVolume{CHUNK_SIZE, BASE}
        , _root{std::make_unique<Node<T>>()}
    {}

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Octree<T>::addPoint(vec3f position, T t) -> Node<T> * {
        return _root->insert(position, t, _boundingVolume);
    }

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Octree<T>::removePoint(vec3f point) -> void {
        _root->removePoint(point, _boundingVolume);
    }

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Octree<T>::cull(const vec3f &position,
                         const Camera::Perspective::Camera &camera,
                         const Renderer::Renderer &renderer) const -> void {
        const Args<T> args = {position, camera, renderer};
        _root->cull(args);
    }

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Octree<T>::find(const vec3f &position) const -> std::optional<Node<T> *> {
        return _root->find(position, _boundingVolume);
    }

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Octree<T>::updateFaceMask() -> u8 {
        return _root->updateFaceMask(_boundingVolume);
    }

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Octree<T>::recombine() -> void {
        std::stack<Node<T> *> stack;
        _root->recombine(stack);
    }

    //
    //
    //

    // ----------------
    // helper functions

    static const u8 indexToSegment[8] = {
            0b10000000U, 0b00001000U, 0b00010000U, 0b00000001U,
            0b01000000U, 0b00000100U, 0b00100000U, 0b00000010U
    };

    // -----------------------------------------------
    // selecting child index based on (x, y, z) vector

    static inline
    auto selectChild(const vec3f &position, const std::pair<u8, vec3f> &currentBVol) -> u8 {
        const auto &[_, root] = currentBVol;

        return ((position.x >= root.x) << 2) |
               ((position.y >= root.y) << 1) |
                (position.z >= root.z);
    }

    // ------------------------------------------------------
    // calculates the new max bounding box for an octree node

    static const i8 indexToPrefix[8][3] = {
            {-1, -1, -1}, {-1, -1, 1}, {-1, 1, -1}, {-1, 1,  1},
            { 1, -1, -1}, { 1, -1, 1}, { 1, 1, -1}, { 1, 1,  1}
    };

    static inline
    auto buildBbox(const u8 index, const std::pair<f32, vec3f> &currentBVol) -> const std::pair<f32, vec3f> {
        const auto &[scale, root] = currentBVol;

        return {
                scale / 2.0F,
                root + glm::vec3((scale / 4.0F) * static_cast<f32>(indexToPrefix[index][0]),
                                 (scale / 4.0F) * static_cast<f32>(indexToPrefix[index][1]),
                                 (scale / 4.0F) * static_cast<f32>(indexToPrefix[index][2]))
        };
    }


    //
    //
    //

    // ---------------------
    // Node implementation

    template<typename T> requires derivedFromBoundingVolume<T>
    Node<T>::Node()
        : _segments{0}
        , _faces{0}
        , _nodes{nullptr}
    {}

    template<typename T> requires derivedFromBoundingVolume<T>
    Node<T>::~Node() noexcept {
        if (!_nodes)
            return;

        if (_segments) {
            for (u8 i = 0; i < 8; ++i) {
                if (_segments & indexToSegment[i])
                    _nodes[i].~Node<T>();
            }

            std::free(_nodes);
        }
        else {
            delete static_cast<BoundingVolume *>(_leaf);
        }
    }

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Node<T>::find(vec3f position, std::pair<f32, vec3f> currentBVol) const -> std::optional<Node<T> *> {
        auto *current = const_cast<Node<T> *>(this);

        while (true) {
            if (!current->_nodes)
                return std::nullopt;

            // -------------------------------------------------------------------------------------
            // spherical approximation of the position

            auto &[scale, root] = current->_boundingVolume;

            if (!current->_segments && (std::pow(glm::distance(position, root), 2) * 2) <= std::pow(scale, 2))
                return std::make_optional(current);

            auto index = selectChild(position, currentBVol);
            if (!(current->_segments & indexToSegment[index]))
                return std::nullopt;

            currentBVol = buildBbox(index, currentBVol);
            current     = &current->_nodes[index];
        }
    }

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Node<T>::insert(vec3f position, T t, std::pair<f32, vec3f> currentBVol) -> Node<T> * {
        Node<T> *current = this;

        while (true) {
            auto& [scale, root] = currentBVol;

            if (scale == BASE_SIZE) {
                current->_boundingVolume = currentBVol;
                current->_leaf = new BoundingVolume {t};
                current->_leaf->_voxelID |= SET_FACES;

                return current;
            }
            else {
                const u8 index   = selectChild(position, currentBVol);
                const u8 segment = indexToSegment[index];

                if (!current->_segments) {
                    current->_boundingVolume = currentBVol;
                    current->_nodes = static_cast<Node<T> *>(
                            std::aligned_alloc(alignof(Node<T>), sizeof(Node<T>) * 8));
                }

                if (!(current->_segments & segment)) {
                    current->_segments ^= segment;
                    current->_nodes[index] = {};
                }

                currentBVol = buildBbox(index, currentBVol);
                current     = &current->_nodes[index];
            }
        }
    }

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Node<T>::removePoint(vec3f position, std::pair<f32, vec3f> currentBVol) -> void {}

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Node<T>::updateFaceMask(const std::pair<f32, vec3f> &currentBVol) -> u8 {
        if (!_nodes)
            return 0;

        for (u8 i = 0; i < 8; ++i) {
            if (_segments & indexToSegment[i]) {
                auto nVec = buildBbox(i, currentBVol);
                _faces |= _nodes[i].updateFaceMask(nVec);
            }
        }

        if (!_segments) {
            _faces = EXTR_FACES(_leaf->_voxelID);
        }

        return _faces;
    }

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Node<T>::recombine(std::stack<Node *> &stack) -> void {
        if (!_segments)
            return;

        stack.push(this);

        for (u8 i = 0; i < 8; ++i)
            if (_segments & indexToSegment[i])
                _nodes[i].recombine(stack);

        // ----------------------------
        // combining same sized volumes

        Node<T> *current;

        while (!stack.empty()) {
            current = stack.top();

            if (current->_segments ^ std::numeric_limits<decltype(current->_segments)>::max())
                return;

            u16 voxelID = 0;
            for (u8 i = 0; i < 8; ++i) {
                if ((current->_nodes[i]._segments) ||
                    (EXTRACT_ID(current->_nodes[i]._leaf->_voxelID) != EXTRACT_ID(current->_nodes[0]._leaf->_voxelID)))
                    return;

                // -----------------------------
                // determine which faces to cull

                voxelID |= current->_nodes[i]._leaf->_voxelID;
            }

            if (SOME_BIT_DIFFERS(2, 3, 6, 7, TOP_BIT)   || SOME_BIT_DIFFERS(0, 2, 4, 6, BACK_BIT) ||
                SOME_BIT_DIFFERS(1, 3, 5, 7, FRONT_BIT) || SOME_BIT_DIFFERS(0, 1, 2, 3, LEFT_BIT) ||
                SOME_BIT_DIFFERS(4, 5, 6, 7, RIGHT_BIT) || SOME_BIT_DIFFERS(0, 1, 4, 5, BOTTOM_BIT))
                return;

            for (u8 i = 0; i < 8; ++i)
                delete current->_nodes[i]._leaf;

            std::free(current->_nodes);

            current->_segments = 0;
            current->_leaf = new BoundingVolume {voxelID};

            stack.pop();
        }
    }

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Node<T>::cull(const Args<T> &args) const -> void {
        const auto &[scale, root] = this->_boundingVolume;

        if (!(_faces & args._camera.getCameraMask()) ||
             (scale > 4 && !args._camera.inFrustum(args._point + root, scale)))
            return;

        if (_segments) {
            for (u8 i = 0; i < 8; ++i) {
                if (_segments & indexToSegment[i])
                    _nodes[i].cull(args);
            }
        }
        else {
            /*
            u16 tmpVoxelID = _leaf->_voxelID;
            _leaf->_voxelID &= (args.camera.getCameraMask() << 10) ^ UINT8_MAX;

            args.renderer.addVoxel(_leaf);

            _leaf->_voxelID = tmpVoxelID;
             */

            auto voxel = BoundingVolumeVoxel {
                static_cast<u16>(_leaf->_voxelID & ((args._camera.getCameraMask() << 10) ^ UINT8_MAX)),
                root,
                scale
            };

            args._renderer.addVoxel(&voxel);
        }
    }

    template class Octree<BoundingVolume>;
    template class Node<BoundingVolume>;
}
