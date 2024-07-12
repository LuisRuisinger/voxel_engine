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
        auto fun = [this](const glm::vec3 position, u8 voxel_ID, Platform *platform) -> void {
            auto normalizedVec = CHUNK_SEGMENT_Y_NORMALIZE(position);
            auto &segment = this->_chunksegments[CHUNK_SEGMENT_Y_DIFF(position)];

            u64 x = static_cast<u8>(normalizedVec.x) & MASK_5;
            u64 y = static_cast<u8>(normalizedVec.y) & MASK_5;
            u64 z = static_cast<u8>(normalizedVec.z) & MASK_5;

            // setting coordinates
            u32 packed_data_highp = (x << 13) |
                                    (y <<  8) |
                                    (z <<  3) |
                                    MASK_3;

            // 12 highest bit set to the index of the chunk inside chunk managing array
            u32 packed_data_lowp  = (this->_chunkIdx << 20) |
                                    (segment._segmentIdx << 16) |
                                    voxel_ID;

            // adding the voxel
            auto *node = segment._segment->addPoint(
                    (static_cast<u64>(packed_data_highp) << SHIFT_HIGH) | packed_data_lowp);
            ++this->_size;

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
            for (u8 x = 0; x < CHUNK_SIZE; ++x)
                for (u8 z = 0; z < CHUNK_SIZE; ++z)
                    for (u8 y = 0; y < 8 + x / 4; ++y)
                        fun(glm::vec3{ x, y, z }, 0, platform);

            // compression
            for (size_t i = 0; i < _chunksegments.size(); ++i) {
                this->_chunksegments[i]._segment->recombine();
                this->_faces |= this->_chunksegments[i]._segment->updateFaceMask((this->_chunkIdx << 4) | i);
                this->_chunksegments[i].initialized = true;
            }
        };

        gen();
    }

    auto Chunk::insert(const glm::vec3 position, u8 voxel_ID, Platform *platform) -> void {
        auto normalizedVec = CHUNK_SEGMENT_Y_NORMALIZE(position);
        auto &segment = this->_chunksegments[CHUNK_SEGMENT_Y_DIFF(position)];

        u64 x = static_cast<u8>(normalizedVec.x) & MASK_5;
        u64 y = static_cast<u8>(normalizedVec.y) & MASK_5;
        u64 z = static_cast<u8>(normalizedVec.z) & MASK_5;

        // setting coordinates
        u32 packed_data_highp = (x << 13) |
                                (y <<  8) |
                                (z <<  3) |
                                MASK_3;

        // 12 highest bit set to the index of the chunk inside chunk managing array
        u32 packed_data_lowp  = (this->_chunkIdx << 20) |
                                (segment._segmentIdx << 16) |
                                voxel_ID;

        // adding the voxel
        auto *node = segment._segment->addPoint(
                (static_cast<u64>(packed_data_highp) << SHIFT_HIGH) | packed_data_lowp);
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
                neighbor->packed_data &=
                        (((neighbor->packed_data >> SHIFT_HIGH) & MASK_3) <=
                         ((current->packed_data  >> SHIFT_HIGH) & MASK_3))
                        ? ~nBit : neighbor->packed_data;
                current->packed_data  &=
                        (((current->packed_data  >> SHIFT_HIGH) & MASK_3) <=
                         ((neighbor->packed_data >> SHIFT_HIGH) & MASK_3))
                        ? ~cBit : current->packed_data;
                break;

            default:
                throw std::runtime_error{"CHUNK::CHUNK_SEGMENT::VOXEL::UNDEFINED_NEIGHBOR_STATE"};
        }
    }

    auto Chunk::find(glm::vec3 position, Platform *platform) -> std::pair<node::Node *, ChunkData> {
        if ((position.x < 0.0F || position.x > CHUNK_SIZE) ||
            (position.z < 0.0F || position.z > CHUNK_SIZE)) {

            // TODO: let chunk store a weak_ptr to neighbouring chunks
            // TODO: use neightbour for occlusion culling

            return { nullptr, EMPTY };
        }
        else {
            auto normalizedVec = CHUNK_SEGMENT_Y_NORMALIZE(position);
            auto &segment = this->_chunksegments[CHUNK_SEGMENT_Y_DIFF(position)];

            u64 x = static_cast<u8>(normalizedVec.x) & MASK_5;
            u64 y = static_cast<u8>(normalizedVec.y) & MASK_5;
            u64 z = static_cast<u8>(normalizedVec.z) & MASK_5;

            auto opt = segment._segment->find((x << 13) | (y << 8) | (z << 3) | MASK_3);
            return opt.has_value()
                ? std::pair { opt.value(), DATA }
                : std::pair { nullptr, EMPTY };
        }
    }

    auto Chunk::remove(glm::vec3 position) -> void {
        auto normalizedVec = CHUNK_SEGMENT_Y_NORMALIZE(position);

        u16 x = static_cast<u8>(normalizedVec.x) & MASK_5;
        u16 y = static_cast<u8>(normalizedVec.y) & MASK_5;
        u16 z = static_cast<u8>(normalizedVec.z) & MASK_5;

        this->_chunksegments[CHUNK_SEGMENT_Y_DIFF(position)]._segment->removePoint((x << 10) | (y << 5) | z);
    }

    auto Chunk::cull(const camera::perspective::Camera &camera, Platform &platform) const -> void {
        auto platformBase = glm::vec3(
                platform.get_world_root().x, 0, platform.get_world_root().y);
        
        auto offset = static_cast<f32>(CHUNK_SIZE) * glm::vec3 {
                static_cast<i32>(this->_chunkIdx % (RENDER_RADIUS * 2)) - RENDER_RADIUS,
                -4,
                static_cast<i32>(this->_chunkIdx / (RENDER_RADIUS * 2)) - RENDER_RADIUS
        };

        auto extracted_faces = std::vector<VERTEX> {};
        for (u8 i = 0; i < this->_chunksegments.size(); ++i) {
            if (this->_chunksegments[i].initialized)
                this->_chunksegments[i]._segment->cull(
                        platformBase + offset + glm::vec3 {
                            0.0F,
                            i * static_cast<f32>(CHUNK_SIZE),
                            0.0F
                        },
                        camera,
                        platform,
                        extracted_faces);
        }

        if (extracted_faces.size() > 0)
            platform.get_presenter().add_voxel_vector(std::move(extracted_faces));
    }

    auto Chunk::update_and_render(
            u16 chunk_idx,
            const core::camera::perspective::Camera &camera,
            Platform &platform)
            -> void {
        this->_chunkIdx = chunk_idx & 0xFFF;

        for (u8 i = 0; i < CHUNK_SEGMENTS; ++i)
            this->_chunksegments[i]._segment->update_chunk_mask((this->_chunkIdx << 4) | i);

        cull(camera, platform);
    }

    auto Chunk::index() const -> u16 {
        return this->_chunkIdx;
    }

    auto Chunk::visible(const camera::perspective::Camera &camera, const Platform &platform) const -> bool {
        const u64 faces = this->_faces & static_cast<u64>(camera.getCameraMask());
        if (!faces || !this->_size)
            return false;

        auto offset = static_cast<f32>(CHUNK_SIZE) * glm::vec2 {
                static_cast<i32>(this->_chunkIdx % (RENDER_RADIUS * 2)) - RENDER_RADIUS,
                static_cast<i32>(this->_chunkIdx / (RENDER_RADIUS * 2)) - RENDER_RADIUS
        };

        return camera.inFrustum(platform.get_world_root() + offset, CHUNK_SIZE);
    }
}