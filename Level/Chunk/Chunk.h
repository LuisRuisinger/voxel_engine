//
// Created by Luis Ruisinger on 18.02.24.
//

#ifndef OPENGL_3D_ENGINE_CHUNK_H
#define OPENGL_3D_ENGINE_CHUNK_H

#include <iostream>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../../global.h"
#include "../Rendering/Renderer.h"
#include "../Level/Octree/Octree.h"
#include "../Level/Octree/Memorypool.h"

#define CHUNK_SEGMENTS 16
#define MIN_HEIGHT (-128)
#define YNORMALIZED_INDEX_OFFSET (MIN_HEIGHT / CHUNK_SIZE)
#define CHUNK_SEGMENT_YOFFS(y) (CHUNK_SIZE * y + MIN_HEIGHT)
#define CHUNK_SEGMENT_YNORMALIZE(_p) (vec3f {(_p).x, static_cast<f32>((static_cast<i32>((_p).y) % CHUNK_SIZE) + 0.5F), (_p).z})
#define CHUNK_SEGMENT_YDIFF(p) (((i32) (p.y - MIN_HEIGHT)) / CHUNK_SIZE)
#define CHUNK_POS_3D(p) (glm::vec3 {p.x, 0, p.y})

namespace Platform{
    class Platform;
}

namespace Chunk {
    enum ChunkData {
        EMPTY, NODATA, DATA
    };

    class Chunk;

    //
    //
    //

    class ChunkSegment {
        friend Chunk;

    public:
        explicit ChunkSegment(vec3f);
        ChunkSegment(ChunkSegment &&other) noexcept;
        ChunkSegment(ChunkSegment &other) = delete;

        ~ChunkSegment() = default;

        auto operator=(ChunkSegment &&other) noexcept -> ChunkSegment &;
        auto operator=(ChunkSegment &other) -> ChunkSegment & = delete;

    private:

        // -------------------------
        // root of the cubic segment

        vec3f _root;

        // -------------------------------------------
        // underlying octree, managing the cubic space

        std::unique_ptr<Octree::Octree<BoundingVolume>> _segment;

        // ------------------------------------------------------------
        // indicator if the segment got manipulated (for serialization)

        bool _modified;
    };

    //
    //
    //

    class Chunk {
    public:
        Chunk(vec2f, Platform::Platform *);
        ~Chunk() = default;

        auto insert(vec3f, BoundingVolume, Platform::Platform *platform) -> void;
        auto remove(vec3f) -> void;
        auto cull  (const Camera::Camera &, const Platform::Platform &) const -> void;
        auto generate(Platform::Platform *position) -> void;
        auto update() -> void;
        auto find(vec3f, Platform::Platform *platform) -> std::pair<Octree::Node<BoundingVolume> *, ChunkData>;
        auto updateOcclusion(Octree::Node<BoundingVolume> *, std::pair<Octree::Node<BoundingVolume> *, ChunkData>, u16, u16) -> void;

        [[nodiscard]] auto getPostion() const -> vec2f;

    private:

        // ------------------------------------------------------
        // a vector of cubic segments which a chunk is split into

        std::vector<ChunkSegment>  _chunksegments;

        // -----------------
        // root of the chunk

        vec2f _root;
    };
}


#endif //OPENGL_3D_ENGINE_CHUNK_H
