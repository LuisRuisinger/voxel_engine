//
// Created by Luis Ruisinger on 18.02.24.
//

#ifndef OPENGL_3D_ENGINE_CHUNK_H
#define OPENGL_3D_ENGINE_CHUNK_H

#include <iostream>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../../util/aliases.h"
#include "../Rendering/Renderer.h"
#include "../Level/Octree/Octree.h"
#include "../Level/Octree/Memorypool.h"

#define CHUNK_SEGMENTS 16
#define MIN_HEIGHT (-128)
#define YNORMALIZED_INDEX_OFFSET (MIN_HEIGHT / CHUNK_SIZE * 2)
#define CHUNK_SEGMENT_YOFFS(y) (CHUNK_SIZE * 0.5 * y + MIN_HEIGHT)
#define CHUNK_SEGMENT_YNORMALIZE(_p) (vec3f {(_p).x, static_cast<f32>((static_cast<i32>((_p).y) % (CHUNK_SIZE / 2)) + 0.5F), (_p).z})
#define CHUNK_SEGMENT_YDIFF(p) (((i32) (p.y - MIN_HEIGHT)) / CHUNK_SIZE * 2)
#define CHUNK_POS_3D(p) (glm::vec3 {p.x, 0, p.y})

namespace Platform{
    class Platform;
}

//
//
//

namespace Chunk {

    //
    //
    //

    class Chunk;

    //
    //
    //

    enum ChunkData {
        EMPTY, NODATA, DATA
    };

    //
    //
    //

    class ChunkSegment {
        friend Chunk;

    public:
        explicit ChunkSegment(u8);
        ChunkSegment(ChunkSegment &&other) noexcept;
        ChunkSegment(ChunkSegment &other) = delete;

        ~ChunkSegment() = default;

        auto operator=(ChunkSegment &&other) noexcept -> ChunkSegment &;
        auto operator=(ChunkSegment &other) -> ChunkSegment & = delete;

    private:
        // -------------------------------------------
        // underlying octree, managing the cubic space

        std::unique_ptr<Octree::Octree<BoundingVolume>> _segment;

        // ------------------------------------------------------------
        // indicator if the segment got manipulated (for serialization)

        bool _modified;

        u8 _segmentIdx;
    };

    //
    //
    //

    class Chunk {
    public:
        Chunk(u16, Platform::Platform *);
        ~Chunk() = default;

        auto insert(vec3f, u8, Platform::Platform *platform) -> void;
        auto remove(vec3f) -> void;
        auto cull(const Camera::Perspective::Camera &, const Platform::Platform &) const -> void;
        auto generate(Platform::Platform *position) -> void;
        auto update(u16) -> void;
        auto find(vec3f, Platform::Platform *platform) -> std::pair<Octree::Node<BoundingVolume> *, ChunkData>;
        auto updateOcclusion(Octree::Node<BoundingVolume> *, std::pair<Octree::Node<BoundingVolume> *, ChunkData>, u64, u64) -> void;
        auto index() const -> u16;

    private:

        // ------------------------------------------------------
        // a vector of cubic segments which a chunk is split into

        std::vector<ChunkSegment>  _chunksegments;

        u16 _chunkIdx;
    };
}


#endif //OPENGL_3D_ENGINE_CHUNK_H