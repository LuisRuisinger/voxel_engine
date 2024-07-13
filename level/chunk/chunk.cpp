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
        : chunk_idx{static_cast<u16>(chunkIdentifier & 0xFFF)}
    {
        for (u8 i = 0; i < CHUNK_SEGMENTS; ++i)
            this->chunk_segments.emplace_back(i);
    }

    auto Chunk::generate(Platform *platform) -> void {
        auto fun = [this](const glm::vec3 position, u8 voxel_ID, Platform *platform) -> void {
            auto normalized_vec = CHUNK_SEGMENT_Y_NORMALIZE(position);
            auto &segment = this->chunk_segments[CHUNK_SEGMENT_Y_DIFF(position)];

            u64 x = static_cast<u8>(normalized_vec.x) & MASK_5;
            u64 y = static_cast<u8>(normalized_vec.y) & MASK_5;
            u64 z = static_cast<u8>(normalized_vec.z) & MASK_5;

            // setting coordinates
            u32 packed_data_highp = (x << 13) |
                                    (y <<  8) |
                                    (z <<  3) |
                                    MASK_3;

            // 12 highest bit set to the index of the chunk inside chunk managing array
            u32 packed_data_lowp  = (this->chunk_idx << 20) |
                                    (segment.segment_idx << 16) |
                                    voxel_ID;

            auto *node = segment.root->addPoint(
                    (static_cast<u64>(packed_data_highp) << SHIFT_HIGH) | packed_data_lowp);
            ++this->size;

            // occlusion culling
            updateOcclusion(node, find(position - glm::vec3 {1, 0, 0}, platform), LEFT_BIT, RIGHT_BIT);
            updateOcclusion(node, find(position + glm::vec3 {1, 0, 0}, platform), RIGHT_BIT, LEFT_BIT);
            updateOcclusion(node, find(position - glm::vec3 {0, 1, 0}, platform), BOTTOM_BIT, TOP_BIT);
            updateOcclusion(node, find(position + glm::vec3 {0, 1, 0}, platform), TOP_BIT, BOTTOM_BIT);
            updateOcclusion(node, find(position - glm::vec3 {0, 0, 1}, platform), BACK_BIT, FRONT_BIT);
            updateOcclusion(node, find(position + glm::vec3 {0, 0, 1}, platform), FRONT_BIT, BACK_BIT);
        };

        auto generator = [&fun, &platform, this]() -> void {
            for (u8 x = 0; x < CHUNK_SIZE; ++x)
                for (u8 z = 0; z < CHUNK_SIZE; ++z)
                    for (u8 y = 0; y < 8 + x / 4; ++y)
                        fun(glm::vec3{ x, y, z }, 0, platform);

            // compression
            for (size_t i = 0; i < chunk_segments.size(); ++i) {
                this->chunk_segments[i].root->recombine();
                this->faces |= this->chunk_segments[i].root->updateFaceMask((this->chunk_idx << 4) | i);
                this->chunk_segments[i].initialized = true;
            }
        };

        generator();
    }

    auto Chunk::insert(const glm::vec3 position, u8 voxel_ID, Platform *platform) -> void {
        auto normalized_vec = CHUNK_SEGMENT_Y_NORMALIZE(position);
        auto &segment = this->chunk_segments[CHUNK_SEGMENT_Y_DIFF(position)];

        u64 x = static_cast<u8>(normalized_vec.x) & MASK_5;
        u64 y = static_cast<u8>(normalized_vec.y) & MASK_5;
        u64 z = static_cast<u8>(normalized_vec.z) & MASK_5;

        // setting coordinates
        u32 packed_data_highp = (x << 13) |
                                (y <<  8) |
                                (z <<  3) |
                                MASK_3;

        // 12 highest bit set to the index of the chunk inside chunk managing array
        u32 packed_data_lowp  = (this->chunk_idx << 20) |
                                (segment.segment_idx << 16) |
                                voxel_ID;

        auto *node = segment.root->addPoint(
                (static_cast<u64>(packed_data_highp) << SHIFT_HIGH) | packed_data_lowp);
        ++this->size;

        // occlusion culling
        updateOcclusion(node, find(position - glm::vec3 {1, 0, 0}, platform), LEFT_BIT, RIGHT_BIT);
        updateOcclusion(node, find(position + glm::vec3 {1, 0, 0}, platform), RIGHT_BIT, LEFT_BIT);
        updateOcclusion(node, find(position - glm::vec3 {0, 1, 0}, platform), BOTTOM_BIT, TOP_BIT);
        updateOcclusion(node, find(position + glm::vec3 {0, 1, 0}, platform), TOP_BIT, BOTTOM_BIT);
        updateOcclusion(node, find(position - glm::vec3 {0, 0, 1}, platform), BACK_BIT, FRONT_BIT);
        updateOcclusion(node, find(position + glm::vec3 {0, 0, 1}, platform), FRONT_BIT, BACK_BIT);

        // recombining voxels
        segment.root->recombine();
        segment.chunk_modified = true;
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
            auto normalized_vec = CHUNK_SEGMENT_Y_NORMALIZE(position);
            auto &segment = this->chunk_segments[CHUNK_SEGMENT_Y_DIFF(position)];

            u64 x = static_cast<u8>(normalized_vec.x) & MASK_5;
            u64 y = static_cast<u8>(normalized_vec.y) & MASK_5;
            u64 z = static_cast<u8>(normalized_vec.z) & MASK_5;

            auto opt = segment.root->find((x << 13) | (y << 8) | (z << 3) | MASK_3);
            return opt.has_value()
                ? std::pair { opt.value(), DATA }
                : std::pair { nullptr, EMPTY };
        }
    }

    auto Chunk::remove(glm::vec3 position) -> void {
        auto normalized_vec = CHUNK_SEGMENT_Y_NORMALIZE(position);

        u16 x = static_cast<u8>(normalized_vec.x) & MASK_5;
        u16 y = static_cast<u8>(normalized_vec.y) & MASK_5;
        u16 z = static_cast<u8>(normalized_vec.z) & MASK_5;

        this->chunk_segments[CHUNK_SEGMENT_Y_DIFF(position)].root->removePoint((x << 10) | (y << 5) | z);
    }

    auto Chunk::cull(const camera::perspective::Camera &camera, Platform &platform) const -> void {
        auto global_root = glm::vec3(
                platform.get_world_root().x, 0, platform.get_world_root().y);
        
        auto offset = static_cast<f32>(CHUNK_SIZE) * glm::vec3 {
                static_cast<i32>(this->chunk_idx % (RENDER_RADIUS * 2)) - RENDER_RADIUS,
                -4,
                static_cast<i32>(this->chunk_idx / (RENDER_RADIUS * 2)) - RENDER_RADIUS
        };

        auto extracted_faces = std::vector<VERTEX> {};
        for (u8 i = 0; i < this->chunk_segments.size(); ++i) {
            if (this->chunk_segments[i].initialized)
                this->chunk_segments[i].root->cull(
                        global_root + offset + glm::vec3 {
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
            u16 nchunk_idx,
            const core::camera::perspective::Camera &camera,
            Platform &platform)
            -> void {
        this->chunk_idx = nchunk_idx & 0xFFF;

        for (u8 i = 0; i < CHUNK_SEGMENTS; ++i)
            this->chunk_segments[i].root->update_chunk_mask((this->chunk_idx << 4) | i);

        cull(camera, platform);
    }

    auto Chunk::index() const -> u16 {
        return this->chunk_idx;
    }

    auto Chunk::visible(const camera::perspective::Camera &camera, const Platform &platform) const -> bool {
        const u64 face_mask = this->faces & static_cast<u64>(camera.getCameraMask());
        if (!face_mask || !this->size)
            return false;

        auto offset = static_cast<f32>(CHUNK_SIZE) * glm::vec2 {
                static_cast<i32>(this->chunk_idx % (RENDER_RADIUS * 2)) - RENDER_RADIUS,
                static_cast<i32>(this->chunk_idx / (RENDER_RADIUS * 2)) - RENDER_RADIUS
        };

        return camera.inFrustum(platform.get_world_root() + offset, CHUNK_SIZE);
    }
}