//
// Created by Luis Ruisinger on 18.02.24.
//

#include <random>

#include "../Platform.h"

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

    Chunk::Chunk(vec2f point, Platform::Platform *platform)
        : chunksegments()
        , position{point}
    {
        for (u8 i = 0; i < CHUNK_SEGMENTS; ++i)
            this->chunksegments.push_back(ChunkSegment {
                CHUNK_POS_3D(this->position) + vec3f {0, i - 4, 0}
            });

        generate(platform);
    }

    auto Chunk::insert(const vec3f point, const BoundingVolume bVol, Platform::Platform *platform) -> void {
        auto *voxel = this->chunksegments[CHUNK_SEGMENT_YDIFF(point)].segment->addPoint(CHUNK_SEGMENT_YNORMALIZE(point), bVol);

        // -----------------
        // occlusion culling

        updateOcclusion(voxel, find(point - vec3f {1, 0, 0}, platform), LEFT_BIT, RIGHT_BIT);
        updateOcclusion(voxel, find(point + vec3f {1, 0, 0}, platform), RIGHT_BIT, LEFT_BIT);
        updateOcclusion(voxel, find(point - vec3f {0, 1, 0}, platform), BOTTOM_BIT, TOP_BIT);
        updateOcclusion(voxel, find(point + vec3f {0, 1, 0}, platform), TOP_BIT, BOTTOM_BIT);
        updateOcclusion(voxel, find(point - vec3f {0, 0, 1}, platform), BACK_BIT, FRONT_BIT);
        updateOcclusion(voxel, find(point + vec3f {0, 0, 1}, platform), FRONT_BIT, BACK_BIT);

        // ---------------------------------------------------------------
        // the recombination should happen at the last stage of generation

        this->chunksegments[CHUNK_SEGMENT_YDIFF(point)].segment->recombine();
        this->chunksegments[CHUNK_SEGMENT_YDIFF(point)].modified = true;
    }

    inline
    auto Chunk::updateOcclusion(BoundingVolume *current,
                                std::pair<BoundingVolume *, ChunkData> pair, u16 cBit, u16 nBit) -> void {
        auto &[neighbor, type] = pair;

        switch (type) {
            case EMPTY:
                break;

            case NODATA:
                current->voxelID &= ~cBit;
                break;

            case DATA:
                neighbor->voxelID &= (neighbor->scale <= current->scale) ? ~nBit : neighbor->voxelID;
                current->voxelID  &= (current->scale <= neighbor->scale) ? ~cBit : current->voxelID;
                break;

            default:
                throw std::runtime_error{"UNDEFINED NEIGHBOR STATE"};
        }
    }

    auto Chunk::find(vec3f point, Platform::Platform *platform) -> std::pair<BoundingVolume *, ChunkData> {
        if ((point.x < 0.0F || point.x > CHUNK_SIZE) ||
            (point.z < 0.0F || point.z > CHUNK_SIZE)) {

            // ------------------
            // outside this chunk

            return {nullptr, EMPTY};
        }
        else {
            auto opt = this->chunksegments[CHUNK_SEGMENT_YDIFF(point)].segment->find(CHUNK_SEGMENT_YNORMALIZE(point));
            return opt.has_value() ? std::pair {opt.value(), DATA} : std::pair {nullptr, EMPTY};
        }
    }

    auto Chunk::remove(vec3f point) -> void {
        this->chunksegments[CHUNK_SEGMENT_YDIFF(point)].segment->removePoint(point);
    }

    auto Chunk::cull(const Camera::Camera &camera, const Platform::Platform &platform) const -> void {
        if (!camera.inFrustum(this->position + platform.getBase(), CHUNK_SIZE))
            return;

        auto globalBase = vec3f {platform.getBase().x, 0, platform.getBase().y};

        for (u8 i = 0; i < CHUNK_SEGMENTS; ++i)
            chunksegments[i].segment->cull(
                    globalBase + chunksegments[i].position * vec3f {CHUNK_SIZE},
                    camera,
                    platform.getRenderer());
    }

    auto Chunk::getPostion() const -> vec2f {
        return this->position;
    }

    auto Chunk::generate(Platform::Platform *platform) -> void {
        auto baseOffset = vec3f {0.5F};

        auto fun = [this](const vec3f point, const BoundingVolume bVol, Platform::Platform *platform) -> void {
            auto *voxel = this->chunksegments[CHUNK_SEGMENT_YDIFF(point)].segment->addPoint(CHUNK_SEGMENT_YNORMALIZE(point), bVol);

            // -----------------
            // occlusion culling

            updateOcclusion(voxel, find(point - vec3f {1, 0, 0}, platform), LEFT_BIT, RIGHT_BIT);
            updateOcclusion(voxel, find(point + vec3f {1, 0, 0}, platform), RIGHT_BIT, LEFT_BIT);
            updateOcclusion(voxel, find(point - vec3f {0, 1, 0}, platform), BOTTOM_BIT, TOP_BIT);
            updateOcclusion(voxel, find(point + vec3f {0, 1, 0}, platform), TOP_BIT, BOTTOM_BIT);
            updateOcclusion(voxel, find(point - vec3f {0, 0, 1}, platform), BACK_BIT, FRONT_BIT);
            updateOcclusion(voxel, find(point + vec3f {0, 0, 1}, platform), FRONT_BIT, BACK_BIT);
        };

        // ----------
        // generation

        for (u8 x = 0; x < CHUNK_SIZE; ++x) {
            for (u8 y = 0; y < 4; ++y) {
                for (u8 z = 0; z < CHUNK_SIZE; ++z) {
                    auto point = vec3f {x, y, z};
                    fun(baseOffset + point, BoundingVolume {0, BASE_SIZE, baseOffset + point}, platform);
                }
            }
        }

        // -----------
        // compression

        for (u8 i = 0; i < CHUNK_SEGMENTS; ++i)
            chunksegments[i].segment->recombine();
    }

    auto Chunk::update() -> void {
        for (u8 i = 0U; i < CHUNK_SEGMENTS; ++i)
            this->chunksegments[i].segment->updateFaceMask();
    }
}