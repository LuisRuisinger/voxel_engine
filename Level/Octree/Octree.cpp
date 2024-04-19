//
// Created by Luis Ruisinger on 17.02.24.
//

#include <immintrin.h>

#include "Octree.h"
#include "../Chunk/Chunk.h"
#include "../Quadtree/Quadtree.h"

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
#define SOME_BIT_DIFFERS(_i1, _i2, _i3, _i4, _bit) ((cur->nodes[(_i1)].bVol->voxelID ^ cur->nodes[(_i2)].bVol->voxelID) & (_bit) || \
                                                    (cur->nodes[(_i1)].bVol->voxelID ^ cur->nodes[(_i3)].bVol->voxelID) & (_bit) || \
                                                    (cur->nodes[(_i1)].bVol->voxelID ^ cur->nodes[(_i4)].bVol->voxelID) & (_bit))

namespace Octree {

    //
    //
    //

    // ----------------------
    // Handler implementation

    template<typename T> requires derivedFromBoundingVolume<T>
    Handler<T>::Handler(vec3f position)
        : position{position}
        , bVec{CHUNK_SIZE, BASE}
        , octree{std::make_unique<Octree<T>>()} {}

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Handler<T>::addPoint(vec3f point, T t, Quadtree::Base ref) -> void {
        this->octree->insert(point, t, this->bVec, ref);
    }

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Handler<T>::removePoint(vec3f point) -> void {
        this->octree->removePoint(point, this->bVec);
    }

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Handler<T>::cull(const vec3f &point,
                          const Camera::Camera &camera,
                          const Renderer::Renderer &renderer) const -> void {
        const Args<T> args = {point, camera, renderer};
        this->octree->cull(args);
    }

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Handler<T>::find(const vec3f &point, Quadtree::Base &ref) -> std::optional<std::pair<T *, ChunkData>> {
        return this->octree->find(point, this->bVec, ref);
    }

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Handler<T>::updateFaceMask() -> u8 {
        return this->octree->updateFaceMask(this->bVec);
    }

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Handler<T>::recombine() -> void {
        std::stack<Octree<T> *> stack;
        this->octree->recombine(stack);
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
    auto selectChild(const vec3f &point, const std::pair<u8, vec3f> &bVec) -> u8 {
        const auto &ref = std::get<1>(bVec);

        return ((point.x >= ref.x) << 2) | ((point.y >= ref.y) << 1) | (point.z >= ref.z);
    }

    // ------------------------------------------------------
    // calculates the new max bounding box for an octree node

    static const i8 indexToPrefix[8][3] = {
            {-1, -1, -1}, {-1, -1, 1}, {-1, 1, -1}, {-1, 1,  1},
            { 1, -1, -1}, { 1, -1, 1}, { 1, 1, -1}, { 1, 1,  1}
    };

    static inline
    auto buildBbox(const u8 index, const std::pair<f32, vec3f> &bVec) -> const std::pair<f32, vec3f> {
        const auto &[scale, point] = bVec;

        return {
                scale / 2.0F,
                point + glm::vec3((scale / 4.0F) * static_cast<f32>(indexToPrefix[index][0]),
                                  (scale / 4.0F) * static_cast<f32>(indexToPrefix[index][1]),
                                  (scale / 4.0F) * static_cast<f32>(indexToPrefix[index][2]))
        };
    }


    //
    //
    //

    // ---------------------
    // Octree implementation

    template<typename T> requires derivedFromBoundingVolume<T>
    Octree<T>::Octree()
        : segments{0}
        , faces{0}
        , nodes{nullptr} {}

    template<typename T> requires derivedFromBoundingVolume<T>
    Octree<T>::~Octree() noexcept {
        if (this->nodes)
            return;

        if (this->segments) {
            for (u8 i = 0; i < 8; ++i) {
                if (this->segments & indexToSegment[i])
                    this->nodes[i].~Octree<T>();
            }

            std::free(this->nodes);
        }
        else {
            delete static_cast<BoundingVolume *>(this->bVol);
        }
    }

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Octree<T>::find(vec3f point,
                         std::pair<f32, vec3f> bVec,
                         Quadtree::Base &ref) const -> std::optional<std::pair<T *, ChunkData>> {
        if (point.y < 0 || point.y > CHUNK_SIZE &&
            !((point.x < 0 || point.x > CHUNK_SIZE) || (point.z < 0 || point.z > CHUNK_SIZE))) {

            auto segment = ref.chunk->find(NEIGHBOR_SEGMENT(ref.chunkSegPos, point));
            if (!segment.has_value())
                return std::make_optional(std::pair {nullptr, NODATA});

            return segment.value()->segment->find(NEIGHBOR_CHUNK_SEGMENT_COORD(point), ref);
        }

        if ((point.x < 0.0F || point.x > CHUNK_SIZE) ||
            (point.y < 0.0F || point.y > CHUNK_SIZE) ||
            (point.z < 0.0F || point.z > CHUNK_SIZE)) {

            if (glm::distance(NEIGHBOR_CHUNK(ref.chunkSegPos, point), ref.handler->getPosition())
            >= RENDER_RADIUS * CHUNK_SIZE)
                return std::make_optional(std::pair {nullptr, NODATA});

            auto chunk = ref.handler->find(NEIGHBOR_CHUNK(ref.chunkSegPos, point));
            if (!chunk.has_value())
                return std::make_optional(std::pair {nullptr, NODATA});

            auto segment = chunk.value()->find(NEIGHBOR_SEGMENT(ref.chunkSegPos, point));
            return segment.value()->segment->find(NEIGHBOR_CHUNK_COORD(point), ref);
        }

        auto *cur = (Octree<T> *) this;
        while (true) {
            if (!cur->nodes)
                return {};

            // ------------------------------------------
            // spherical approximation of bounding volume

            if (!cur->segments &&
                glm::distance(point, cur->bVol->position) <= (f32) (cur->bVol->scale >> 1) * sqrt(2.0))
                return std::make_optional(std::pair {cur->bVol, DATA});

            auto index = selectChild(point, bVec);
            if (!(cur->segments & indexToSegment[index]))
                return {};

            bVec = buildBbox(index, bVec);
            cur  = &cur->nodes[index];
        }
    }

    template<typename T> requires derivedFromBoundingVolume<T>
    static inline
    auto updateOcclusion(BoundingVolume *current, std::optional<std::pair<T *, ChunkData>> neighbor,
                         u16 currentbit, u16 neighborBit) -> void {
        if (!neighbor.has_value())
            return;

        if (neighbor.value().first) {
            if (neighbor.value().first->scale <= current->scale)
                neighbor.value().first->voxelID &= ~neighborBit;

            if (neighbor.value().first->scale >= current->scale)
                current->voxelID &= ~currentbit;
        }
        else if (neighbor.value().second == NODATA)
            current->voxelID &= ~currentbit;
    }

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Octree<T>::insert(vec3f point, T t, std::pair<f32, vec3f> bVec, Quadtree::Base &ref) -> void {
        Octree *cur = this;

        while (true) {
            auto& [scale, position] = bVec;

            if (scale == BASE_SIZE) {
                cur->bVol = new BoundingVolume {std::move(t)};
                cur->bVol->voxelID |= 0x3F << 10;

                // ------------------------------------
                // face culling and updating neighbours

                updateOcclusion(cur->bVol, find(point - vec3f {1, 0, 0}, CHUNK_BVEC, ref), LEFT_BIT, RIGHT_BIT);
                updateOcclusion(cur->bVol, find(point + vec3f {1, 0, 0}, CHUNK_BVEC, ref), RIGHT_BIT, LEFT_BIT);
                updateOcclusion(cur->bVol, find(point - vec3f {0, 1, 0}, CHUNK_BVEC, ref), BOTTOM_BIT, TOP_BIT);
                updateOcclusion(cur->bVol, find(point + vec3f {0, 1, 0}, CHUNK_BVEC, ref), TOP_BIT, BOTTOM_BIT);
                updateOcclusion(cur->bVol, find(point - vec3f {0, 0, 1}, CHUNK_BVEC, ref), BACK_BIT, FRONT_BIT);
                updateOcclusion(cur->bVol, find(point + vec3f {0, 0, 1}, CHUNK_BVEC, ref), FRONT_BIT, BACK_BIT);

                return;
            }
            else {
                const u8 index   = selectChild(point, bVec);
                const u8 segment = indexToSegment[index];

                if (!cur->segments) {
                    cur->bVbec = bVec;
                    cur->nodes = static_cast<Octree<T> *>(
                            std::aligned_alloc(alignof(Octree<T>), sizeof(Octree<T>) * 8));
                }

                if (!(cur->segments & segment)) {
                    cur->segments ^= segment;
                    cur->nodes[index] = Octree<T>{};
                }

                bVec = buildBbox(index, bVec);
                cur  = &cur->nodes[index];
            }
        }
    }

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Octree<T>::removePoint(vec3f point, std::pair<f32, vec3f> bVec) -> void {}

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Octree<T>::updateFaceMask(const std::pair<f32, vec3f> &bVec) -> u8 {
        if (!this->nodes)
            return 0;

        if (!this->segments) {
            this->faces = EXTR_FACES(this->bVol->voxelID);
        }
        else {
            for (u8 i = 0; i < 8; ++i) {
                if (this->segments & indexToSegment[i]) {
                    const auto &nVec = buildBbox(i, bVec);
                    this->faces |= this->nodes[i].updateFaceMask(nVec);
                }
            }
        }

        return this->faces;
    }

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Octree<T>::recombine(std::stack<Octree *> &stack) -> void {
        if (!this->segments)
            return;

        /*
        if (ONE_BIT_SET_ONLY(this->segments)) {
            u8 segment = SET_BIT_INDEX(this->segments);
            u8 idx = 0;

            for (u8 i = 0; i < 8; ++i)
                if (indexToSegment[i] & segment) {
                    idx = i;
                    break;
                }

            this->segments = this->nodes[idx].segments;
            this->bVbec    = this->nodes[idx].bVbec;

            Octree<T> *tmp = this->nodes;
            this->nodes = this->nodes[idx].nodes;

            this->nodes[idx].nodes = nullptr;
            tmp[idx].~Octree<T>();
            std::free(tmp);
        }
         */

        stack.push(this);

        for (u8 i = 0; i < 8; ++i)
            if (this->segments & indexToSegment[i])
                this->nodes[i].recombine(stack);

        // ----------------------------
        // combining same sized volumes

        Octree<T> *cur;

        while (!stack.empty()) {
            cur = stack.top();

            if (cur->segments ^ UINT8_MAX)
                return;

            u16 voxelID = 0;
            for (u8 i = 0; i < 8; ++i) {
                if ((cur->nodes[i].segments) ||
                    (EXTRACT_ID(cur->nodes[i].bVol->voxelID) != EXTRACT_ID(cur->nodes[0].bVol->voxelID)) ||
                    (cur->nodes[i].bVol->scale != cur->nodes[0].bVol->scale))
                    return;

                // -----------------------------
                // determine which faces to cull

                voxelID |= cur->nodes[i].bVol->voxelID;
            }

            if (SOME_BIT_DIFFERS(2, 3, 6, 7, TOP_BIT)   || SOME_BIT_DIFFERS(0, 2, 4, 6, BACK_BIT) ||
                SOME_BIT_DIFFERS(1, 3, 5, 7, FRONT_BIT) || SOME_BIT_DIFFERS(0, 1, 2, 3, LEFT_BIT) ||
                SOME_BIT_DIFFERS(4, 5, 6, 7, RIGHT_BIT) || SOME_BIT_DIFFERS(0, 1, 4, 5, BOTTOM_BIT))
                return;

            u8 scale = cur->nodes[0].bVol->scale;

            vec3f &posMin = cur->nodes[0].bVol->position;
            vec3f &posMax = cur->nodes[7].bVol->position;
            vec3f  newPos = posMin + glm::abs(posMax - posMin) * 0.5f;

            for (u8 i = 0; i < 8; ++i)
                delete cur->nodes[i].bVol;

            std::free(cur->nodes);

            cur->segments = 0;
            cur->bVol = new BoundingVolume {
                .voxelID  = voxelID,
                .scale    = (u8) (scale * 2),
                .position = newPos
            };

            stack.pop();
        }
    }

    template<typename T> requires derivedFromBoundingVolume<T>
    auto Octree<T>::cull(const Args<T> &args) const -> void {
        if (!this->nodes || !(this->faces & args.camera.getCameraMask()))
            return;

        if (this->segments) {
            const auto& [scale, position] = this->bVbec;

            if ((scale > 4 && !args.camera.inFrustum(args.point + position, (u32) (scale))))
                return;

            for (u8 i = 0; i < 8; ++i) {
                if (this->segments & indexToSegment[i])
                    this->nodes[i].cull(args);
            }
        }
        else {
            this->bVol->position += args.point;

            u16 tmpVoxelID  = this->bVol->voxelID;
            this->bVol->voxelID &= (args.camera.getCameraMask() << 10) ^ UINT8_MAX;

            args.renderer.addVoxel(this->bVol);

            this->bVol->position -= args.point;
            this->bVol->voxelID = tmpVoxelID;
        }
    }

    template class Handler<BoundingVolume>;
    template class Octree<BoundingVolume>;
}
