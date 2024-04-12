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
#define CHUNK_SEGMENT_YDIFF(p) (((i32) (p.y - MIN_HEIGHT)) / CHUNK_SIZE)
#define CHUNK_POS_3D(p) (glm::vec3 {p.x, 0, p.y})

namespace Quadtree {
    class Handler;
    struct Base;
}

namespace Chunk {
    struct ChunkSegment {
        explicit ChunkSegment(vec3f);
        ~ChunkSegment() = default;
        ChunkSegment(ChunkSegment &&) noexcept;
        auto operator=(ChunkSegment &&) noexcept -> ChunkSegment &;

        vec3f position;
        std::unique_ptr<Octree::Handler<BoundingVolume>> segment;
        bool modified;
    };
    class Chunk {
    public:
        Chunk(vec2f, Quadtree::Handler *);
        ~Chunk() = default;

        auto insert(vec3f, BoundingVolume) -> void;
        auto remove(vec3f) -> void;
        auto cull  (const Camera::Camera &, const Renderer::Renderer &) const -> void;
        auto generate() -> void;
        auto update() -> u8;
        auto find(vec3f) ->std::optional<ChunkSegment *>;

        [[nodiscard]] auto getPostion() const -> vec2f;

    private:
        std::vector<ChunkSegment>  chunksegments;
        vec2f                      position;
        Quadtree::Handler         *handler;
    };
}


#endif //OPENGL_3D_ENGINE_CHUNK_H
