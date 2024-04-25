//
// Created by Luis Ruisinger on 18.02.24.
//

#include <random>

#include "../Platform.h"

namespace Chunk {

    //
    //
    //

    ChunkSegment::ChunkSegment(vec3f root)
        : _root{root}
        , _modified{false}
        , _segment{std::make_unique<Octree::Octree<BoundingVolume>>(vec3f(0))}
    {}

    ChunkSegment::ChunkSegment(ChunkSegment &&other) noexcept
        : _root{other._root}
        , _modified{other._modified}
        , _segment{std::move(other._segment)}
    {}

    auto ChunkSegment::operator=(ChunkSegment &&other) noexcept -> ChunkSegment & {
        _root = other._root;
        _modified = other._modified;
        _segment  = std::move(other._segment);

        other._modified = false;

        return *this;
    }

    //
    //
    //

    Chunk::Chunk(vec2f root, Platform::Platform *platform)
        : _chunksegments{}
        , _root{root}
    {
        for (u8 i = 0; i < CHUNK_SEGMENTS; ++i)
            _chunksegments.emplace_back(ChunkSegment {
                CHUNK_POS_3D(_root) + vec3f {0, i + YNORMALIZED_INDEX_OFFSET, 0}
            });

        generate(platform);
    }

    auto Chunk::insert(const vec3f position, 
                       const BoundingVolume boundingVolume, 
                       Platform::Platform *platform) -> void {
        auto *node = _chunksegments[CHUNK_SEGMENT_YDIFF(position)]._segment->addPoint(
                CHUNK_SEGMENT_YNORMALIZE(position),
                boundingVolume);

        // -----------------
        // occlusion culling

        updateOcclusion(node, find(position - vec3f {1, 0, 0}, platform), LEFT_BIT, RIGHT_BIT);
        updateOcclusion(node, find(position + vec3f {1, 0, 0}, platform), RIGHT_BIT, LEFT_BIT);
        updateOcclusion(node, find(position - vec3f {0, 1, 0}, platform), BOTTOM_BIT, TOP_BIT);
        updateOcclusion(node, find(position + vec3f {0, 1, 0}, platform), TOP_BIT, BOTTOM_BIT);
        updateOcclusion(node, find(position - vec3f {0, 0, 1}, platform), BACK_BIT, FRONT_BIT);
        updateOcclusion(node, find(position + vec3f {0, 0, 1}, platform), FRONT_BIT, BACK_BIT);

        // ---------------------------------------------------------------
        // the recombination should happen at the last stage of generation

        _chunksegments[CHUNK_SEGMENT_YDIFF(position)]._segment->recombine();
        _chunksegments[CHUNK_SEGMENT_YDIFF(position)]._modified = true;
    }

    inline
    auto Chunk::updateOcclusion(Octree::Node<BoundingVolume> *current,
                                std::pair<Octree::Node<BoundingVolume> *, ChunkData> pair, u16 cBit, u16 nBit) -> void {
        auto &[neighbor, type] = pair;

        switch (type) {
            case EMPTY:
                break;

            case NODATA:
                current->_leaf->_voxelID &= ~cBit;
                break;

            case DATA:
                neighbor->_leaf->_voxelID &= (std::get<0>(neighbor->_boundingVolume) <= 
                                              std::get<0>(current->_boundingVolume))
                        ? ~nBit : neighbor->_leaf->_voxelID;
                current->_leaf->_voxelID  &= (std::get<0>(current->_boundingVolume) <= 
                                              std::get<0>(neighbor->_boundingVolume))
                        ? ~cBit : current->_leaf->_voxelID;
                break;

            default:
                throw std::runtime_error{"CHUNK::CHUNK_SEGMENT::VOXEL::UNDEFINED_NEIGHBOR_STATE"};
        }
    }

    auto Chunk::find(vec3f position,
                     Platform::Platform *platform) -> std::pair<Octree::Node<BoundingVolume> *, ChunkData> {
        if ((position.x < 0.0F || position.x > CHUNK_SIZE) ||
            (position.z < 0.0F || position.z > CHUNK_SIZE)) {

            // ------------------
            // outside this chunk

            // TODO

            return {nullptr, EMPTY};
        }
        else {
            auto opt = _chunksegments[CHUNK_SEGMENT_YDIFF(position)]._segment->find(CHUNK_SEGMENT_YNORMALIZE(position));
            return opt.has_value() ? std::pair {opt.value(), DATA} : std::pair {nullptr, EMPTY};
        }
    }

    auto Chunk::remove(vec3f point) -> void {
        _chunksegments[CHUNK_SEGMENT_YDIFF(point)]._segment->removePoint(point);
    }

    auto Chunk::cull(const Camera::Perspective::Camera &camera, const Platform::Platform &platform) const -> void {
        if (!camera.inFrustum(platform.getBase() + _root * vec2f {CHUNK_SIZE}, CHUNK_SIZE))
            return;

        auto globalBase = vec3f {platform.getBase().x, 0, platform.getBase().y};

        for (u8 i = 0; i < CHUNK_SEGMENTS; ++i) {
            platform.getRenderer().addChunk(_chunksegments[i]._root);
            _chunksegments[i]._segment->cull(
                    globalBase + _chunksegments[i]._root * vec3f {CHUNK_SIZE},
                    camera,
                    platform.getRenderer());
        }
    }

    auto Chunk::position() const -> vec2f {
        return _root;
    }

    auto Chunk::generate(Platform::Platform *platform) -> void {
        auto fun = [this](
                const vec3f position,
                const BoundingVolume boundingVolume,
                Platform::Platform *platform) -> void {
            auto *node = _chunksegments[CHUNK_SEGMENT_YDIFF(position)]._segment->addPoint(
                    CHUNK_SEGMENT_YNORMALIZE(position),
                    boundingVolume);

            // -----------------
            // occlusion culling

            updateOcclusion(node, find(position - vec3f {1, 0, 0}, platform), LEFT_BIT, RIGHT_BIT);
            updateOcclusion(node, find(position + vec3f {1, 0, 0}, platform), RIGHT_BIT, LEFT_BIT);
            updateOcclusion(node, find(position - vec3f {0, 1, 0}, platform), BOTTOM_BIT, TOP_BIT);
            updateOcclusion(node, find(position + vec3f {0, 1, 0}, platform), TOP_BIT, BOTTOM_BIT);
            updateOcclusion(node, find(position - vec3f {0, 0, 1}, platform), BACK_BIT, FRONT_BIT);
            updateOcclusion(node, find(position + vec3f {0, 0, 1}, platform), FRONT_BIT, BACK_BIT);
        };

        // ----------
        // generation

        for (u8 x = 0; x < CHUNK_SIZE; ++x) {
            for (u8 y = 0; y < 4; ++y) {
                for (u8 z = 0; z < CHUNK_SIZE; ++z) {
                    auto point = vec3f {x, y, z};
                    fun(point, BoundingVolume {0}, platform);
                }
            }
        }

        for (u8 x = 0; x < CHUNK_SIZE; ++x) {
            for (u8 y = 5; y < 6; ++y) {
                for (u8 z = 0; z < CHUNK_SIZE; ++z) {
                    auto point = vec3f {x, y, z};
                    fun(point, BoundingVolume {0}, platform);
                }
            }
        }

        // -----------
        // compression

        for (u8 i = 0; i < CHUNK_SEGMENTS; ++i)
            _chunksegments[i]._segment->recombine();
    }

    auto Chunk::update() -> void {
        for (u8 i = 0U; i < CHUNK_SEGMENTS; ++i)
            _chunksegments[i]._segment->updateFaceMask();
    }
}