//
// Created by Luis Ruisinger on 14.03.24.
//

#include "Quadtree.h"

namespace Quadtree {

    //
    //
    //

    // ----------------------
    // Handler implementation

    Handler::Handler(glm::vec2 position)
            : position{position}, bVec{RENDER_RADIUS * CHUNK_SIZE * 2, position}
            , quadtree{std::make_unique<Quadtree>()}
    {}

    auto Handler::insert(glm::vec3 point, uint16_t voxelID) -> void {
        this->quadtree->insert(point, voxelID, this->bVec);
    }

    auto Handler::insertChunk(const glm::vec2 point) -> void {
        this->quadtree->insertChunk(point, this->bVec, this);
    }

    auto Handler::extractChunk(const glm::vec2 point) -> Chunk::Chunk* {
        return this->quadtree->extractChunk(point, this->bVec);
    }

    auto Handler::insertExtractedChunk(Chunk::Chunk* chunk) -> void {
        this->quadtree->insertExtractedChunk(chunk, this->bVec);
    }

    auto Handler::remove(glm::vec3 point) -> void {
        this->quadtree->remove(point, this->bVec);
    }

    auto Handler::removeChunk(glm::vec3 point) -> void {
        this->quadtree->removeChunk(point, this->bVec);
    }

    auto Handler::cull(const Camera::Camera& camera, Renderer::Renderer& renderer) const -> void {
        this->quadtree->cull(camera, this->bVec, renderer);
    }

    auto Handler::find(const vec2f point) const -> std::optional<Chunk::Chunk *> {
        return this->quadtree->find(point, this->bVec);
    }

    auto Handler::update() -> void {
        static_cast<void>(this->quadtree->update());
    }

    auto Handler::getPosition() -> vec2f {
        return this->position;
    }

    //
    //
    //

    // ----------------
    // helper functions

    static const u8 indexToSegment[4] = {
            0b10000000U, 0b00010000U, 0b01000000U, 0b00100000U
    };

    static const i8 indexToPrefix[4][2] = {
            {-1, -1}, {-1, 1}, {1, -1}, {1, 1}
    };

    // -----------------------------------------------
    // selecting child index based on (x, y, z) vector

    static inline
    auto selectChild(const vec2f &point, const std::pair<u8, vec2f> &bVec) -> u8 {
        return (static_cast<u8>(point.x >= std::get<1>(bVec).x) << 1) | static_cast<u8>(point.y >= std::get<1>(bVec).y);
    }

    // ------------------------------------------------------
    // calculates the new max bounding box for an octree node

    static inline
    auto buildBbox(const u8 index, const std::pair<f32, vec2f> &bVec) -> const std::pair<f32, vec2f> {
        const auto &[scale, point] = bVec;

        return {
                scale / 2.0F,
                point + glm::vec2((scale / 4.0F) * static_cast<f32>(indexToPrefix[index][0]),
                                  (scale / 4.0F) * static_cast<f32>(indexToPrefix[index][1]))
        };
    }

    //
    //
    //

    // -----------------------
    // Quadtree implementation

    Quadtree::Quadtree() noexcept
        : children{nullptr}
        , faces{0U}
        , segments{0U}
    {}

    Quadtree::~Quadtree() {
        if (!this->children) {
            return;
        }
        else if (this->segments) {
            delete[] static_cast<Quadtree *>(this->children);
        }
        else {
            delete static_cast<Chunk::Chunk *>(this->children);
        }
    }

    auto Quadtree::insert(glm::vec3 point, uint16_t voxelID,
                          const std::pair<uint16_t, glm::vec2> &bVec) -> void {}

    auto Quadtree::insertChunk(const glm::vec2 point,
                               const std::pair<uint16_t, glm::vec2> &bVec,
                               Handler *handler) -> void {
        if (std::get<0>(bVec) <= CHUNK_SIZE) {
            this->children = new Chunk::Chunk{point, handler};
        }
        else {
            u8 index = selectChild(point, bVec);
            std::pair nVec = buildBbox(index, bVec);

            if (!this->segments)
                this->children = new Quadtree[4U];

            if (!(this->segments & indexToSegment[index]))
                this->segments |= indexToSegment[index];

            ((Quadtree *) this->children)[index].insertChunk(point, nVec, handler);
        }

    }

    auto Quadtree::extractChunk(const glm::vec2 point,
                                const std::pair<uint16_t, glm::vec2> &bVec) -> Chunk::Chunk * {
        if (std::get<0>(bVec) <= CHUNK_SIZE) {
            auto *chunk = static_cast<Chunk::Chunk *>(this->children);
            this->children = nullptr;

            return chunk;
        }
        else {
            uint8_t index  = selectChild(point, bVec);
            std::pair nVec = buildBbox(index, bVec);

            return static_cast<Quadtree *>(this->children)[index].extractChunk(point, nVec);
        }
    }

    auto Quadtree::insertExtractedChunk(const Chunk::Chunk *chunk,
                                        const std::pair<uint16_t, glm::vec2> &bVec) -> void {
        if (std::get<0>(bVec) <= CHUNK_SIZE) {
            this->children = (void*) chunk;
        }
        else {
            glm::vec2 chunkPos = chunk->getPostion();
            uint8_t index      = selectChild(chunkPos, bVec);
            std::pair nVec     = buildBbox(index, bVec);

            if (!this->segments)
                this->children = (void *) new Quadtree[4U];

            if (!(this->segments & indexToSegment[index])) {
                this->segments |= indexToSegment[index];
            }

            ((Quadtree *) this->children)[index].insertExtractedChunk(chunk, nVec);
        }
    }

    auto Quadtree::remove(glm::vec3 point, const std::pair<uint16_t, glm::vec2>& bVec) -> void {
        if (std::get<0>(bVec) <= CHUNK_SIZE) {
            if (this->children)
                static_cast<Chunk::Chunk*>(this->children)->remove(point);
        }
        else {
            uint8_t index = selectChild(point, bVec);
            std::pair nVec = buildBbox(index, bVec);

            if (!this->segments || !(this->segments & indexToSegment[index]))
                return;

            static_cast<Quadtree*>(this->children)[index].remove(point, bVec);
        }
    }

    auto Quadtree::removeChunk(glm::vec3 point, const std::pair<uint16_t, glm::vec2> &bVec) -> void {
        if (std::get<0>(bVec) <= CHUNK_SIZE) {
            if (this->children)
                delete static_cast<Chunk::Chunk *>(this->children);
        }
        else {
            uint8_t index = selectChild(point, bVec);
            std::pair nVec = buildBbox(index, bVec);

            if (!this->segments || !(this->segments & indexToSegment[index]))
                return;

            static_cast<Quadtree*>(this->children)[index].removeChunk(point, bVec);
        }
    }

    auto Quadtree::cull(const Camera::Camera& camera,
                        const std::pair<uint16_t, glm::vec2> &bVec, Renderer::Renderer& renderer) -> void {
        auto& [scale, point] = bVec;

        /*
         * seems like &'ing with the camera here is bad ??
         */

        if (!this->children || !this->faces || !camera.inFrustum(point, scale))
            return;

        if (scale <= CHUNK_SIZE) {
            ((Chunk::Chunk *) this->children)->cull(camera, renderer);
        }
        else {
            for (u8 i = 0U; i < 4U; ++i) {
                if (this->segments & indexToSegment[i]) {
                    std::pair nVec = buildBbox(i, bVec);
                    ((Quadtree *) this->children)[i].cull(camera, nVec, renderer);
                }
            }
        }
    }

    auto Quadtree::find(const vec2f position,
                        const std::pair<u16, vec2f> &bVec) const -> std::optional<Chunk::Chunk *> {
        if (!this->children)
            return {};

        auto& [scale, _] = bVec;
        if (scale <= CHUNK_SIZE)
            return std::make_optional((Chunk::Chunk *) this->children);

        u8 index = selectChild(position, bVec);
        if (!(this->segments & indexToSegment[index]))
                return {};

        std::pair nVec = buildBbox(index, bVec);
        return ((Quadtree *) this->children)[index].find(position, nVec);
    }

    auto Quadtree::update() -> u8 {
        if (!this->children)
            return 0;

        if (!this->segments) {
            this->faces = ((Chunk::Chunk *) this->children)->update();
        }
        else {
            for (uint8_t i = 0; i < 4; ++i)
                if (this->segments & indexToSegment[i])
                    this->faces |= ((Quadtree *) this->children)[i].update();
        }

        return this->faces;
    }
}