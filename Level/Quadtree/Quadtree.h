//
// Created by Luis Ruisinger on 14.03.24.
//

#ifndef OPENGL_3D_ENGINE_QUADTREE_H
#define OPENGL_3D_ENGINE_QUADTREE_H

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdlib>

#include "../../global.h"
#include "../../camera.h"
#include "../../Rendering/Renderer.h"
#include "../Chunk/Chunk.h"

#define RENDER_RADIUS 7
#define RENDER_DISTANCE (RENDER_RADIUS * CHUNK_SIZE)

namespace Quadtree {
    class Quadtree {
    public:
        Quadtree() noexcept;
        ~Quadtree();

        auto insert(vec3f, u16, const std::pair<u16, vec2f> &) -> void;

        auto insertChunk(const glm::vec2, const std::pair<u16, vec2f>&, Handler *) -> void;

        auto remove(vec3f, const std::pair<u16, vec2f> &) -> void;

        auto removeChunk(vec3f, const std::pair<u16, vec2f> &) -> void;

        auto cull(const Camera::Camera &, const std::pair<u16, vec2f> &, Renderer::Renderer &) -> void;

        auto extractChunk(const vec2f, const std::pair<u16, vec2f> &) -> Chunk::Chunk *;

        auto insertExtractedChunk(const Chunk::Chunk *, const std::pair<u16, vec2f> &) -> void;
        auto find(const vec2f point, const std::pair<u16, vec2f> &) const -> std::optional<Chunk::Chunk *>;
        auto update() -> u8;

    private:

        // --------------------------------------------------
        // segments identify child nodes in use or chunk leaf

        u8    segments;
        u8    faces;
        void *children;
    };

    class Handler {
    public:
        Handler(vec2f);

        ~Handler() = default;

        // ---------------------------------------------------------------------------
        // inserts a voxel with scale 1 into the platform, searches for fitting octree

        auto insert(vec3f, u16) -> void;

        // -------------------------------------------------------
        // inserts an entire chunk at the calculated quadtree node

        auto insertChunk(vec2f) -> void;

        // ---------------------------------------------------------------------------
        // removes a voxel with scale 1 into the platform, searches for fitting octree

        auto remove(vec3f) -> void;

        // -------------------------------------------------------
        // removes an entire chunk at the calculated quadtree node

        auto removeChunk(vec3f) -> void;

        // ---------------------------------------------------------------------------------------
        // performes multiple culling techniques and writes visible faces into the renderer buffer

        auto cull(const Camera::Camera &, Renderer::Renderer &) const -> void;

        // --------------------------------------------------------------------------------------------------
        // extracts an entire chunk at the calculated quadtree node, the tree won't contain the chunk anymore

        auto extractChunk(vec2f) -> Chunk::Chunk*;

        // ----------------------------------------------------------
        // inserts an extracted chunk at the calculated quadtree node

        auto insertExtractedChunk(Chunk::Chunk *chunk) -> void;

        auto find(const vec2f point) const -> std::optional<Chunk::Chunk *>;

        auto update() -> void;

        auto getPosition() -> vec2f;

    private:

        std::unique_ptr<Quadtree>        quadtree;

        const vec2f                      position;
        const std::pair<uint16_t, vec2f> bVec;
    };

    struct Base {
        Handler      *handler;
        Chunk::Chunk *chunk;
        vec3f        &chunkSegPos;
    };
}

#endif //OPENGL_3D_ENGINE_QUADTREE_H
