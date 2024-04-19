//
// Created by Luis Ruisinger on 18.02.24.
//

#include <random>

#include "Chunk.h"
#include "../Quadtree/Quadtree.h"

namespace Chunk {

    //
    //
    //

    ChunkSegment::ChunkSegment(vec3f point)
        : position{point}
        , modified{false}
        , segment{std::make_unique<Octree::Handler<BoundingVolume>>(vec3f(0))}
    {}

    ChunkSegment::ChunkSegment(ChunkSegment &&other) noexcept
        : position{other.position}
        , modified{other.modified}
        , segment{std::move(other.segment)}
    {}

    auto ChunkSegment::operator=(ChunkSegment &&other) noexcept -> ChunkSegment & {
        this->position = other.position;
        this->modified = other.modified;
        this->segment  = std::move(other.segment);

        other.modified = false;

        return *this;
    }

    //
    //
    //

    Chunk::Chunk(vec2f point, Quadtree::Handler *handler)
        : chunksegments()
        , handler{handler}
        , position{point}
    {
        for (u8 i = 0; i < CHUNK_SEGMENTS; ++i)
            this->chunksegments.emplace_back(
                    CHUNK_POS_3D(this->position) + vec3f {0, CHUNK_SEGMENT_YOFFS(i), 0});

        generate();
    }

    auto Chunk::insert(const vec3f point, const BoundingVolume bVol) -> void {
        /*
        this->chunksegments[CHUNK_SEGMENT_YDIFF(point)].segment->addPoint(
                point, bVol, Quadtree::Base {
                    this->handler,
                    this,
                    this->chunksegments[CHUNK_SEGMENT_YDIFF(point)].position
                });

        this->chunksegments[CHUNK_SEGMENT_YDIFF(point)].modified = true;
         */
    }

    auto Chunk::remove(vec3f point) -> void {
        this->chunksegments[CHUNK_SEGMENT_YDIFF(point)].segment->removePoint(point);
    }

    auto Chunk::cull(const Camera::Camera &camera, const Renderer::Renderer &renderer) const -> void {
        for (u8 i = 0; i < CHUNK_SEGMENTS; ++i)
            chunksegments[i].segment->cull(chunksegments[i].position, camera, renderer);
    }

    auto Chunk::getPostion() const -> vec2f {
        return this->position;
    }

    auto Chunk::generate() -> void {
        for (u8 x = 0; x < CHUNK_SIZE; ++x) {
            for (u8 z = 0; z < CHUNK_SIZE; ++z) {
                for (u8 y = 0; y < 4; ++y) {
                    auto p = vec3f { x + 0.5, y + 0.5, z + 0.5 };
                    this->chunksegments[4].segment->addPoint(
                            p,
                            BoundingVolume {
                                    0,
                                    BASE_SIZE,
                                    p
                            },
                            Quadtree::Base {
                                    this->handler,
                                    this,
                                    this->chunksegments[4U].position
                            });
                }
            }
        }

        this->chunksegments[4U].segment->recombine();
    }

    auto Chunk::find(const vec3f point) -> std::optional<ChunkSegment *> {
        auto index = CHUNK_SEGMENT_YDIFF(point);

        return (index < 0 || index > CHUNK_SEGMENTS - 1)
        ? std::nullopt
        : std::make_optional(&this->chunksegments[index]);
    }

    auto Chunk::update() -> u8 {
        u8 mask = 0;
        for (u8 i = 0U; i < CHUNK_SEGMENTS; ++i)
            mask |= this->chunksegments[i].segment->updateFaceMask();

        return mask;
    }
}