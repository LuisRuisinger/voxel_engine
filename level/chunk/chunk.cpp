// Created by Luis Ruisinger on 18.02.24.
//

#include <random>

#include "chunk.h"
#include "../presenter.h"
#include "../../util/assert.h"
#include "../../util/perlin_noise.hpp"

namespace core::level::chunk {
    auto Chunk::Faces::operator[](u64 mask) -> size_t & {
        switch (mask) {
            case TOP_BIT   : return this->stored_faces[0];
            case BOTTOM_BIT: return this->stored_faces[1];
            case FRONT_BIT : return this->stored_faces[2];
            case BACK_BIT  : return this->stored_faces[3];
            case LEFT_BIT  : return this->stored_faces[4];
            case RIGHT_BIT : return this->stored_faces[5];
        }
    }

    Chunk::Chunk(u16 chunkIdentifier)
        : chunk_idx{static_cast<u16>(chunkIdentifier & 0xFFF)}
    {
        for (u8 i = 0; i < CHUNK_SEGMENTS; ++i)
            this->chunk_segments.emplace_back(i);
    }

    auto Chunk::generate(Platform *platform) -> void {
        for (u8 x = 0; x < CHUNK_SIZE; ++x)
            for (u8 z = 0; z < CHUNK_SIZE; ++z)
                if (x == CHUNK_SIZE - 1 && x == z) {
                    for (u8 y = 0; y < CHUNK_SIZE; ++y)
                        insert(glm::vec3{ x, y, z }, 0, platform, false);
                }
                else {
                    for (u8 y = 0; y < 8 + x / 4; ++y)
                        insert(glm::vec3{ x, y, z }, 0, platform, false);
                }

        // compression
        for (size_t i = 0; i < chunk_segments.size(); ++i) {
            // this->chunk_segments[i].root->recombine();
            this->faces |= this->chunk_segments[i].root->updateFaceMask((this->chunk_idx << 4) | i);
            this->chunk_segments[i].initialized = true;

            this->mask_container[TOP_BIT]    += this->chunk_segments[i].root->count_mask(TOP_BIT);
            this->mask_container[BOTTOM_BIT] += this->chunk_segments[i].root->count_mask(BOTTOM_BIT);
            this->mask_container[FRONT_BIT]  += this->chunk_segments[i].root->count_mask(FRONT_BIT);
            this->mask_container[BACK_BIT]   += this->chunk_segments[i].root->count_mask(BACK_BIT);
            this->mask_container[LEFT_BIT]   += this->chunk_segments[i].root->count_mask(LEFT_BIT);
            this->mask_container[RIGHT_BIT]  += this->chunk_segments[i].root->count_mask(RIGHT_BIT);
        }
    }

    auto Chunk::insert(
            const glm::vec3 position,
            u8 voxel_ID, Platform *platform,
            bool recombine) -> void {
        auto normalized_vec = CHUNK_SEGMENT_Y_NORMALIZE(position);
        auto &segment = this->chunk_segments[CHUNK_SEGMENT_Y_DIFF(position)];

        u64 x = static_cast<u8>(normalized_vec.x) & MASK_5;
        u64 y = static_cast<u8>(normalized_vec.y) & MASK_5;
        u64 z = static_cast<u8>(normalized_vec.z) & MASK_5;

        // setting coordinates
        u32 packed_data_highp = (x << 13) | (y <<  8) | (z <<  3) | MASK_3;

        // 12 highest bit set to the index of the chunk inside chunk managing array
        u32 packed_data_lowp  =
                (this->chunk_idx << 20) |
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
        if (recombine) {
            segment.root->recombine();
            segment.chunk_modified = true;
        }
    }

    inline
    auto Chunk::updateOcclusion(
            node::Node *current,
            node::Node *neighbor,
            u64 cBit,
            u64 nBit) -> void {
        if (neighbor) {
            if (((neighbor->packed_data >> SHIFT_HIGH) & MASK_3) <=
                ((current->packed_data  >> SHIFT_HIGH) & MASK_3))
                neighbor->packed_data &= ~nBit;

            if (((current->packed_data  >> SHIFT_HIGH) & MASK_3) <=
                ((neighbor->packed_data >> SHIFT_HIGH) & MASK_3))
                current->packed_data &= ~cBit;
        }
    }

    auto Chunk::find(glm::vec3 position, Platform *platform) -> node::Node * {

        // inter-chunk-finding
        if (position.x >= CHUNK_SIZE) {
            for (const auto &[p, w] : this->neighbors)
                if (p == Position::FRONT) {
                    if (auto ptr = w.lock()) {
                        position.x -= CHUNK_SIZE;
                        ASSERT_EQ(ptr.get());
                        return ptr->find(position, platform);
                    }
                }
        }
        else if (position.x < 0.0F) {
            for (const auto &[p, w] : this->neighbors)
                if (p == Position::BACK) {
                    if (auto ptr = w.lock()) {
                        position.x += CHUNK_SIZE;
                        ASSERT_EQ(ptr.get());
                        return ptr->find(position, platform);
                    }
                }
        }
        else if (position.z >= CHUNK_SIZE) {
            for (const auto& [p, w] : this->neighbors)
                if (p == Position::RIGHT) {
                    if (auto ptr = w.lock()) {
                        position.z -= CHUNK_SIZE;
                        ASSERT_EQ(ptr.get());
                        return ptr->find(position, platform);
                    }
                }
        }
        else if (position.z < 0.0F) {
            for (const auto& [p, w] : this->neighbors)
                if (p == Position::LEFT) {
                    if (auto ptr = w.lock()) {
                        position.z += CHUNK_SIZE;
                        ASSERT_EQ(ptr.get());
                        return ptr->find(position, platform);
                    }
                }
        }

        // intra-chunk-finding
        else if ((position.x >= 0 && position.x < CHUNK_SIZE) &&
                 (position.z >= 0 && position.z < CHUNK_SIZE)) {
            auto normalized_vec = CHUNK_SEGMENT_Y_NORMALIZE(position);
            auto &segment = this->chunk_segments[CHUNK_SEGMENT_Y_DIFF(position)];

            u64 x = static_cast<u8>(normalized_vec.x) & MASK_5;
            u64 y = static_cast<u8>(normalized_vec.y) & MASK_5;
            u64 z = static_cast<u8>(normalized_vec.z) & MASK_5;

            return segment.root->find((x << 13) | (y << 8) | (z << 3) | MASK_3);
        }

        // in case this neighbor does not exist
        return nullptr;
    }

    auto Chunk::remove(glm::vec3 position) -> void {
        auto normalized_vec = CHUNK_SEGMENT_Y_NORMALIZE(position);

        u16 x = static_cast<u8>(normalized_vec.x) & MASK_5;
        u16 y = static_cast<u8>(normalized_vec.y) & MASK_5;
        u16 z = static_cast<u8>(normalized_vec.z) & MASK_5;

        this->chunk_segments[CHUNK_SEGMENT_Y_DIFF(position)].root->removePoint(
                (x << 10) | (y << 5) | z);
    }

    auto Chunk::cull(
            const camera::perspective::Camera &camera,
            Platform &platform) const -> void {
        auto global_root = glm::vec3 {
                platform.get_world_root().x,
                0,
                platform.get_world_root().y
        };

        auto offset = static_cast<f32>(CHUNK_SIZE) * glm::vec3 {
                static_cast<i32>(this->chunk_idx % (RENDER_RADIUS * 2)) - RENDER_RADIUS,
                -4,
                static_cast<i32>(this->chunk_idx / (RENDER_RADIUS * 2)) - RENDER_RADIUS
        };

        auto sum = 0;
        for (u64 i = 0; i < 6; ++i)
            if (camera.getCameraMask() & i)
                sum += this->mask_container[i << 50];

        if (!sum)
            return;

        auto *buffer =
                platform.get_presenter().request_writeable_area(
                        sum,
                        threading::worker_id);

        u64 actual_size = 0;
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
                        buffer,
                        actual_size);
        }

        ASSERT_EQ(actual_size <= sum);
        platform.get_presenter().add_size_writeable_area(
                actual_size,
                threading::worker_id);
    }

    auto Chunk::update_and_render(
            u16 nchunk_idx,
            const core::camera::perspective::Camera &camera,
            Platform &platform) -> void {
        std::remove_if(
                this->neighbors.begin(),
                this->neighbors.end(),
                [](const auto& ref) -> bool {
            const auto &[p, w] = ref;
            return w.expired();
        });

        this->chunk_idx = nchunk_idx & 0xFFF;
        for (u8 i = 0; i < CHUNK_SEGMENTS; ++i)
            this->chunk_segments[i].root->update_chunk_mask((this->chunk_idx << 4) | i);

        cull(camera, platform);
    }

    auto Chunk::index() const -> u16 {
        return this->chunk_idx;
    }

    auto Chunk::visible(
            const camera::perspective::Camera &camera,
            const Platform &platform) const -> bool {
        const u64 face_mask = this->faces & static_cast<u64>(camera.getCameraMask());
        if (!face_mask || !this->size)
            return false;

        auto offset = static_cast<f32>(CHUNK_SIZE) * glm::vec2 {
                static_cast<i32>(this->chunk_idx % (RENDER_RADIUS * 2)) - RENDER_RADIUS,
                static_cast<i32>(this->chunk_idx / (RENDER_RADIUS * 2)) - RENDER_RADIUS
        };

        return camera.inFrustum(platform.get_world_root() + offset, CHUNK_SIZE);
    }

    auto Chunk::add_neigbor(Position position, std::shared_ptr<Chunk> neighbor) -> void {
        for (auto &[p, w] : this->neighbors)
            if (p == position && !w.expired()) {
                DEBUG_LOG("Neighbour candidate failure");
                w = std::move(neighbor);
                return;
            }

        this->neighbors.emplace_back(position, std::move(neighbor));
    }
}
