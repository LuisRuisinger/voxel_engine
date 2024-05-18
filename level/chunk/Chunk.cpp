//
// Created by Luis Ruisinger on 18.02.24.
//

#include <random>

#include "../Platform.h"

namespace Chunk {
    constexpr const u64 setFaces = static_cast<u64>(0x3F) << 50;
    constexpr const u64 setScale = static_cast<u64>(0x7)  << 32;

    //
    //
    //

    ChunkSegment::ChunkSegment(u8 segmentIdx)
        : _segmentIdx{segmentIdx}
        , _modified{false}
        , _segment{std::make_unique<Octree::Octree<BoundingVolume>>()}
    {}

    ChunkSegment::ChunkSegment(ChunkSegment &&other) noexcept
        : _segmentIdx{other._segmentIdx}
        , _modified{other._modified}
        , _segment{std::move(other._segment)}
    {}

    auto ChunkSegment::operator=(ChunkSegment &&other) noexcept -> ChunkSegment & {
        _segmentIdx = other._segmentIdx;
        _modified   = other._modified;
        _segment    = std::move(other._segment);

        other._modified = false;
        return *this;
    }

    //
    //
    //

    Chunk::Chunk(u16 chunkIdentifier, Platform::Platform *platform)
        : _chunksegments{}
        , _chunkIdx{static_cast<u16>(chunkIdentifier & 0xFFF)}
    {
        for (u8 i = 0; i < CHUNK_SEGMENTS; ++i)
            _chunksegments.emplace_back(ChunkSegment(i));

        generate(platform);
    }

    auto Chunk::insert(const vec3f position, 
                       u8 voxelID,
                       Platform::Platform *platform) -> void {
        auto  normalizedVec = CHUNK_SEGMENT_YNORMALIZE(position);
        auto &segment       = _chunksegments[CHUNK_SEGMENT_YDIFF(position)];

        u64 x = static_cast<u8>(normalizedVec.x) & 0x1F;
        u64 y = static_cast<u8>(normalizedVec.y) & 0x1F;
        u64 z = static_cast<u8>(normalizedVec.z) & 0x1F;

        // setting coordinates
        u32 packedDataHighP = (x << 13) | (y << 8) | (z << 3) |

                              // setting scale to the biggest possible bounding volume
                              0x7;

        // 12 highest bit set to the index of the chunk inside chunk managing array
        u32 packedDataLowP  = (_chunkIdx << 20) |

                              // 4 bits set to the segment index
                              (segment._segmentIdx << 16) |

                              // the identifier of the voxel
                              voxelID;

        // adding the voxel
        auto *node = segment._segment->addPoint((static_cast<u64>(packedDataHighP) << 32) | packedDataLowP);

        // occlusion culling
        updateOcclusion(node, find(position - vec3f {1, 0, 0}, platform), LEFT_BIT, RIGHT_BIT);
        updateOcclusion(node, find(position + vec3f {1, 0, 0}, platform), RIGHT_BIT, LEFT_BIT);
        updateOcclusion(node, find(position - vec3f {0, 1, 0}, platform), BOTTOM_BIT, TOP_BIT);
        updateOcclusion(node, find(position + vec3f {0, 1, 0}, platform), TOP_BIT, BOTTOM_BIT);
        updateOcclusion(node, find(position - vec3f {0, 0, 1}, platform), BACK_BIT, FRONT_BIT);
        updateOcclusion(node, find(position + vec3f {0, 0, 1}, platform), FRONT_BIT, BACK_BIT);

        // recombining voxels
        // segment._segment->recombine();
        segment._modified = true;
    }

    inline
    auto Chunk::updateOcclusion(Octree::Node<BoundingVolume> *current,
                                std::pair<Octree::Node<BoundingVolume> *, ChunkData> pair,
                                u64 cBit,
                                u64 nBit) -> void {
        auto &[neighbor, type] = pair;

        switch (type) {
            case EMPTY:
                break;

            case NODATA:
                current->_packed &= ~cBit;
                break;

            case DATA:
                neighbor->_packed &= (((neighbor->_packed >> 32) & 0x7) <= ((current->_packed >> 32) & 0x7))
                        ? ~nBit : neighbor->_packed;
                current->_packed  &= (((current->_packed >> 32) & 0x7) <= ((neighbor->_packed >> 32) & 0x7))
                        ? ~cBit : current->_packed;
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
            auto  normalizedVec = CHUNK_SEGMENT_YNORMALIZE(position);
            auto &segment       = _chunksegments[CHUNK_SEGMENT_YDIFF(position)];

            u64 x = static_cast<u8>(normalizedVec.x) & 0x1F;
            u64 y = static_cast<u8>(normalizedVec.y) & 0x1F;
            u64 z = static_cast<u8>(normalizedVec.z) & 0x1F;

            auto opt = segment._segment->find((x << 13) | (y << 8) | (z << 3) | 0x7);
            return opt.has_value() ? std::pair {opt.value(), DATA} : std::pair {nullptr, EMPTY};
        }
    }

    auto Chunk::remove(vec3f position) -> void {
        auto normalizedVec = CHUNK_SEGMENT_YNORMALIZE(position);

        u16 x = static_cast<u8>(normalizedVec.x) & 0x1F;
        u16 y = static_cast<u8>(normalizedVec.y) & 0x1F;
        u16 z = static_cast<u8>(normalizedVec.z) & 0x1F;

        _chunksegments[CHUNK_SEGMENT_YDIFF(position)]._segment->removePoint((x << 10) | (y << 5) | z);
    }

    auto Chunk::cull(const Camera::Perspective::Camera &camera, const Platform::Platform &platform) const -> void {
        auto platformBase = glm::vec3 { platform.getBase().x, 0,platform.getBase().y };
        auto offset = 32.0F * glm::vec3(
                static_cast<i32>(_chunkIdx % (RENDER_RADIUS * 2)) - RENDER_RADIUS,
                -4,
                static_cast<i32>(_chunkIdx / (RENDER_RADIUS * 2)) - RENDER_RADIUS
        );

        for (u8 i = 0; i < CHUNK_SEGMENTS; ++i) {
            _chunksegments[i]._segment->cull(
                    platformBase + offset + glm::vec3(0.0F, i * 32.0F, 0.0F),
                    camera,
                    platform.getRenderer());
        }
    }

    auto Chunk::generate(Platform::Platform *platform) -> void {
        auto fun = [this](const vec3f position, u8 voxelID, Platform::Platform *platform) -> void {
            auto  normalizedVec = CHUNK_SEGMENT_YNORMALIZE(position);
            auto &segment       = _chunksegments[CHUNK_SEGMENT_YDIFF(position)];

            u64 x = static_cast<u8>(normalizedVec.x) & 0x1F;
            u64 y = static_cast<u8>(normalizedVec.y) & 0x1F;
            u64 z = static_cast<u8>(normalizedVec.z) & 0x1F;

                                  // setting coordinates
            u32 packedDataHighP = (x << 13) | (y << 8) | (z << 3) |

                                  // setting scale to the biggest possible bounding volume
                                  0x7;

                                  // 12 highest bit set to the index of the chunk inside chunk managing array
            u32 packedDataLowP  = (_chunkIdx << 20) |

                                  // 4 bits set to the segment index
                                  (segment._segmentIdx << 16) |

                                  // the identifier of the voxel
                                  voxelID;

            // adding the voxel
            auto *node = segment._segment->addPoint((static_cast<u64>(packedDataHighP) << 32) | packedDataLowP);

            // occlusion culling
            updateOcclusion(node, find(position - vec3f {1, 0, 0}, platform), LEFT_BIT, RIGHT_BIT);
            updateOcclusion(node, find(position + vec3f {1, 0, 0}, platform), RIGHT_BIT, LEFT_BIT);
            updateOcclusion(node, find(position - vec3f {0, 1, 0}, platform), BOTTOM_BIT, TOP_BIT);
            updateOcclusion(node, find(position + vec3f {0, 1, 0}, platform), TOP_BIT, BOTTOM_BIT);
            updateOcclusion(node, find(position - vec3f {0, 0, 1}, platform), BACK_BIT, FRONT_BIT);
            updateOcclusion(node, find(position + vec3f {0, 0, 1}, platform), FRONT_BIT, BACK_BIT);
        };

        // generation
        for (u8 x = 0; x < (CHUNK_SIZE / 2); ++x) {
            for (u8 y = 0; y < 1; ++y) {
                for (u8 z = 0; z < (CHUNK_SIZE / 2); ++z) {
                    auto point = vec3f {x, y, z};
                    fun(point, 0, platform);
                }
            }
        }

        // compression
        //for (u8 i = 0; i < CHUNK_SEGMENTS; ++i)
        //    _chunksegments[i]._segment->recombine();
    }

    auto Chunk::update(u16 chunkIdx) -> void {
        _chunkIdx = chunkIdx & 0xFFF;

        for (u8 i = 0; i < CHUNK_SEGMENTS; ++i)
            _chunksegments[i]._segment->updateFaceMask((_chunkIdx << 4) | i);
    }

    auto Chunk::index() const -> u16 {
        return _chunkIdx;
    }
}