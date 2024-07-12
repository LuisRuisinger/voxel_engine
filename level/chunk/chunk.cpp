//
// Created by Luis Ruisinger on 18.02.24.
//

#include <random>

#include "chunk.h"
#include "../presenter.h"
#include "../../util/assert.h"
#include "../../util/perlin_noise.hpp"

namespace core::level::chunk {
    Chunk::Chunk(u16 chunkIdentifier)
        : _chunksegments{}
        , _chunkIdx{static_cast<u16>(chunkIdentifier & 0xFFF)}
        , _faces{0}
        , _size{0}
    {
        for (u8 i = 0; i < CHUNK_SEGMENTS; ++i)
            this->_chunksegments.emplace_back(i);
    }

    auto Chunk::generate(Platform *platform) -> void {
        auto fun = [this](const glm::vec3 position, u8 voxelID, Platform *platform) -> void {
            auto  normalizedVec = CHUNK_SEGMENT_YNORMALIZE(position);
            auto &segment       = this->_chunksegments[CHUNK_SEGMENT_YDIFF(position)];

            u64 x = static_cast<u8>(normalizedVec.x) & 0x1F;
            u64 y = static_cast<u8>(normalizedVec.y) & 0x1F;
            u64 z = static_cast<u8>(normalizedVec.z) & 0x1F;

            // setting coordinates
            u32 packedDataHighP = (x << 13) | (y << 8) | (z << 3) |

                                  // setting scale to the biggest possible bounding volume
                                  0x7;

            // 12 highest bit set to the index of the chunk inside chunk managing array
            u32 packedDataLowP  = (this->_chunkIdx << 20) |

                                  // 4 bits set to the segment index
                                  (segment._segmentIdx << 16) |

                                  // the identifier of the voxel
                                  voxelID;

            // adding the voxel
            auto *node = segment._segment->addPoint((static_cast<u64>(packedDataHighP) << 32) | packedDataLowP);

            // occlusion culling
            updateOcclusion(node, find(position - glm::vec3 {1, 0, 0}, platform), LEFT_BIT, RIGHT_BIT);
            updateOcclusion(node, find(position + glm::vec3 {1, 0, 0}, platform), RIGHT_BIT, LEFT_BIT);
            updateOcclusion(node, find(position - glm::vec3 {0, 1, 0}, platform), BOTTOM_BIT, TOP_BIT);
            updateOcclusion(node, find(position + glm::vec3 {0, 1, 0}, platform), TOP_BIT, BOTTOM_BIT);
            updateOcclusion(node, find(position - glm::vec3 {0, 0, 1}, platform), BACK_BIT, FRONT_BIT);
            updateOcclusion(node, find(position + glm::vec3 {0, 0, 1}, platform), FRONT_BIT, BACK_BIT);
        };

        auto gen = [&fun, &platform, this]() -> void {

            // generation
            for (u8 x = 0; x < (CHUNK_SIZE / 2); ++x) {
                for (u8 z = 0; z < (CHUNK_SIZE / 2); ++z) {
                    for (u8 y = 0; y < 8 + x / 4; ++y) {
                        auto point = glm::vec3{x, y, z};
                        fun(point, 0, platform);

                        ++this->_size;
                    }
                }
            }

            // compression
            for (size_t i = 0; i < _chunksegments.size(); ++i) {
                this->_chunksegments[i]._segment->recombine();
                this->_faces |= this->_chunksegments[i]._segment->updateFaceMask((this->_chunkIdx << 4) | i);
                this->_chunksegments[i].initialized = true;
            }
        };

        gen();
    }

    auto Chunk::destroy() -> void {
        for (size_t i = 0; i < _chunksegments.size(); ++i)
            if (_chunksegments[i]._modified) {}

        _chunksegments.clear();
    }

    auto Chunk::insert(const glm::vec3 position, u8 voxelID, Platform *platform) -> void {
        auto  normalizedVec = CHUNK_SEGMENT_YNORMALIZE(position);
        auto &segment       = this->_chunksegments[CHUNK_SEGMENT_YDIFF(position)];

        u64 x = static_cast<u8>(normalizedVec.x) & 0x1F;
        u64 y = static_cast<u8>(normalizedVec.y) & 0x1F;
        u64 z = static_cast<u8>(normalizedVec.z) & 0x1F;

        // setting coordinates
        u32 packedDataHighP = (x << 13) | (y << 8) | (z << 3) |

                              // setting scale to the biggest possible bounding volume
                              0x7;

        // 12 highest bit set to the index of the chunk inside chunk managing array
        u32 packedDataLowP  = (this->_chunkIdx << 20) |

                              // 4 bits set to the segment index
                              (segment._segmentIdx << 16) |

                              // the identifier of the voxel
                              voxelID;

        // adding the voxel
        auto *node = segment._segment->addPoint((static_cast<u64>(packedDataHighP) << 32) | packedDataLowP);
        ++this->_size;

        // occlusion culling
        updateOcclusion(node, find(position - glm::vec3 {1, 0, 0}, platform), LEFT_BIT, RIGHT_BIT);
        updateOcclusion(node, find(position + glm::vec3 {1, 0, 0}, platform), RIGHT_BIT, LEFT_BIT);
        updateOcclusion(node, find(position - glm::vec3 {0, 1, 0}, platform), BOTTOM_BIT, TOP_BIT);
        updateOcclusion(node, find(position + glm::vec3 {0, 1, 0}, platform), TOP_BIT, BOTTOM_BIT);
        updateOcclusion(node, find(position - glm::vec3 {0, 0, 1}, platform), BACK_BIT, FRONT_BIT);
        updateOcclusion(node, find(position + glm::vec3 {0, 0, 1}, platform), FRONT_BIT, BACK_BIT);

        // recombining voxels
        segment._segment->recombine();
        segment._modified = true;
    }

    inline
    auto Chunk::updateOcclusion(
            node::Node *current,
            std::pair<node::Node *, ChunkData> pair,
            u64 cBit,
            u64 nBit) -> void {
        auto &[neighbor, type] = pair;

        switch (type) {
            case EMPTY:
                break;

            case NODATA:
                current->packed_data &= ~cBit;
                break;

            case DATA:
                neighbor->packed_data &= (((neighbor->packed_data >> 32) & 0x7) <= ((current->packed_data >> 32) & 0x7))
                        ? ~nBit : neighbor->packed_data;
                current->packed_data  &= (((current->packed_data >> 32) & 0x7) <= ((neighbor->packed_data >> 32) & 0x7))
                        ? ~cBit : current->packed_data;
                break;

            default:
                throw std::runtime_error{"CHUNK::CHUNK_SEGMENT::VOXEL::UNDEFINED_NEIGHBOR_STATE"};
        }
    }

    auto Chunk::find(glm::vec3 position, Platform *platform) -> std::pair<node::Node *, ChunkData> {
        if ((position.x < 0.0F || position.x > 32.0F) ||
            (position.z < 0.0F || position.z > 32.0F)) {

            // ------------------
            // outside this chunk

            // TODO

            return {nullptr, EMPTY};
        }
        else {
            auto  normalizedVec = CHUNK_SEGMENT_YNORMALIZE(position);
            auto &segment       = this->_chunksegments[CHUNK_SEGMENT_YDIFF(position)];

            u64 x = static_cast<u8>(normalizedVec.x) & 0x1F;
            u64 y = static_cast<u8>(normalizedVec.y) & 0x1F;
            u64 z = static_cast<u8>(normalizedVec.z) & 0x1F;

            auto opt = segment._segment->find((x << 13) | (y << 8) | (z << 3) | 0x7);
            return opt.has_value()
                ? std::pair {opt.value(), DATA}
                : std::pair {nullptr, EMPTY};
        }
    }

    auto Chunk::remove(glm::vec3 position) -> void {
        auto normalizedVec = CHUNK_SEGMENT_YNORMALIZE(position);

        u16 x = static_cast<u8>(normalizedVec.x) & 0x1F;
        u16 y = static_cast<u8>(normalizedVec.y) & 0x1F;
        u16 z = static_cast<u8>(normalizedVec.z) & 0x1F;

        this->_chunksegments[CHUNK_SEGMENT_YDIFF(position)]._segment->removePoint((x << 10) | (y << 5) | z);
    }

    auto Chunk::cull(const camera::perspective::Camera &camera, Platform &platform) const -> void {
        auto platformBase = glm::vec3(
                platform.get_world_root().x, 0, platform.get_world_root().y);
        
        auto offset = 32.0F * glm::vec3(
                static_cast<i32>(this->_chunkIdx % (RENDER_RADIUS * 2)) - RENDER_RADIUS,
                -4,
                static_cast<i32>(this->_chunkIdx / (RENDER_RADIUS * 2)) - RENDER_RADIUS
        );

        auto extracted_faces = std::vector<VERTEX> {};
        for (u8 i = 0; i < this->_chunksegments.size(); ++i) {
            if (this->_chunksegments[i].initialized)
                this->_chunksegments[i]._segment->cull(
                        platformBase + offset + glm::vec3(0.0F, i * 32.0F, 0.0F),
                        camera,
                        platform,
                        extracted_faces);
        }

        if (extracted_faces.size() > 0)
            platform.get_presenter().add_voxel_vector(std::move(extracted_faces));
    }

    auto Chunk::update_and_render(
            u16 chunkIdx,
            const core::camera::perspective::Camera &camera,
            Platform &platform) -> void {
        this->_chunkIdx = chunkIdx & 0xFFF;

        for (u8 i = 0; i < CHUNK_SEGMENTS; ++i)
            this->_faces |= this->_chunksegments[i]._segment->updateFaceMask((this->_chunkIdx << 4) | i);

        cull(camera, platform);
    }

    auto Chunk::index() const -> u16 {
        return this->_chunkIdx;
    }

    auto Chunk::visible(const camera::perspective::Camera &camera, const Platform &platform) const -> bool {
        const u64 faces = this->_faces & static_cast<u64>(camera.getCameraMask());
        if (!faces || !this->_size)
            return false;

        auto offset = 32.0F * glm::vec2(
                static_cast<i32>(this->_chunkIdx % (RENDER_RADIUS * 2)) - RENDER_RADIUS,
                static_cast<i32>(this->_chunkIdx / (RENDER_RADIUS * 2)) - RENDER_RADIUS
        );

        return camera.inFrustum(platform.get_world_root() + offset, 32);
    }
}