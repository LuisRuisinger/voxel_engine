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
        ~ChunkSegment() = default;

        ChunkSegment(ChunkSegment &&) noexcept;
        ChunkSegment(ChunkSegment &) = delete;

        auto operator=(ChunkSegment &&) noexcept -> ChunkSegment &;
        auto operator=(ChunkSegment &) -> ChunkSegment & = delete;

    private:
        vec3f position;
        std::unique_ptr<Octree::Handler<BoundingVolume>> segment;
        bool modified;
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
        auto generate(Platform::Platform *platform) -> void;
        auto update() -> void;
        auto find(vec3f, Platform::Platform *platform) -> std::pair<Octree::Octree<BoundingVolume> *, ChunkData>;
        auto updateOcclusion(Octree::Octree<BoundingVolume> *, std::pair<Octree::Octree<BoundingVolume> *, ChunkData>, u16, u16) -> void;

        [[nodiscard]] auto getPostion() const -> vec2f;

    private:
        std::vector<ChunkSegment>  chunksegments;
        vec2f                      position;
    };
}


#endif //OPENGL_3D_ENGINE_CHUNK_H
