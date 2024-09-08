// Created by Luis Ruisinger on 18.02.24.
//

#include <random>

#include "chunk.h"
#include "chunk_renderer.h"

#include "../chunk_data_structure/voxel_data_layout.h"

#include "../util/assert.h"
#include "../util/player.h"
#include "../util/tagged_ptr.h"

#include "generation/generation.h"

namespace core::level::chunk {
    auto OcclusionMap::operator[](u64 mask) -> std::unordered_map<node::Node *, u32> & {
        switch (mask) {
            case TOP_BIT   : return this->map[0];
            case BOTTOM_BIT: return this->map[1];
            case FRONT_BIT : return this->map[2];
            case BACK_BIT  : return this->map[3];
            case LEFT_BIT  : return this->map[4];
            case RIGHT_BIT : return this->map[5];
        }
    }

    auto Chunk::Faces::operator[](u64 mask) -> u64 & {
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

    auto Chunk::generate(glm::vec2 root) -> void {

        // generate steps
        auto offset =
                root +
                static_cast<f32>(CHUNK_SIZE) *
                glm::vec2 {
                        static_cast<i32>(this->chunk_idx % (RENDER_RADIUS * 2)) - RENDER_RADIUS,
                        static_cast<i32>(this->chunk_idx / (RENDER_RADIUS * 2)) - RENDER_RADIUS
                };

        generation::generation::Generator::generate(*this, offset);
        for (size_t i = 0; i < chunk_segments.size(); ++i)
            this->faces |= this->chunk_segments[i].root->updateFaceMask(
                    (this->chunk_idx << 4) | i);
    }

    auto Chunk::insert(
            const glm::vec3 position,
            u16 voxel_ID, platform::Platform *platform,
            bool recombine) -> void {
        auto normalized_vec = CHUNK_SEGMENT_Y_NORMALIZE(position);
        auto &segment = this->chunk_segments[CHUNK_SEGMENT_Y_DIFF(position)];

        u64 x = static_cast<u8>(normalized_vec.x) & MASK_5;
        u64 y = static_cast<u8>(normalized_vec.y) & MASK_5;
        u64 z = static_cast<u8>(normalized_vec.z) & MASK_5;

        // setting coordinates
        u32 packed_data_highp = (x << 13) | (y <<  8) | (z <<  3) | MASK_3;

        // 12 highest bit set to the index of the chunk inside chunk managing array
        u32 packed_data_lowp = (this->chunk_idx << 20) |
                               (segment.segment_idx << 16) |
                               (voxel_ID & 0x1FF);

        auto *node = segment.root->addPoint(
                (static_cast<u64>(packed_data_highp) << SHIFT_HIGH) | packed_data_lowp);
        ++this->size;

        f32 offset = 1 << ((node->packed_data >> SHIFT_HIGH) & MASK_3);

        // occlusion culling
        update_occlusion(node, find(position - glm::vec3 {1, 0, 0}, platform), LEFT_BIT, RIGHT_BIT);
        update_occlusion(node, find(position + glm::vec3 {1, 0, 0}, platform), RIGHT_BIT, LEFT_BIT);
        update_occlusion(node, find(position - glm::vec3 {0, 1, 0}, platform), BOTTOM_BIT, TOP_BIT);
        update_occlusion(node, find(position + glm::vec3 {0, 1, 0}, platform), TOP_BIT, BOTTOM_BIT);
        update_occlusion(node, find(position - glm::vec3 {0, 0, 1}, platform), BACK_BIT, FRONT_BIT);
        update_occlusion(node, find(position + glm::vec3 {0, 0, 1}, platform), FRONT_BIT, BACK_BIT);

        // recombining voxels
        if (recombine) {
            segment.root->recombine();
            segment.chunk_modified = true;
        }
    }

    inline
    auto Chunk::update_occlusion(
            node::Node *current,
            node::Node *neighbor,
            u64 current_mask,
            u64 neighbor_mask) -> void {
        if (neighbor) {

            // every voxel we insert has BASE_SIZE and thus is guaranteed
            // to be blocked if a neighbor exists for this face
            current->packed_data &= ~current_mask;

            // the neighbor is a simple BASE_SIZE voxel
            auto neighbor_cube_side = 1 << ((neighbor->packed_data >> SHIFT_HIGH) & MASK_3);
            if (neighbor_cube_side == BASE_SIZE) {
                neighbor->packed_data &= ~neighbor_mask;
                return;
            }

            // the neighbor is bigger than BASE_SIZE
            // we need to check for all other voxels of this quad if it is occluded
            auto &map = this->occlusion_map[neighbor_mask];
            if (map.contains(neighbor)) {
                ++map[neighbor];

                if (map[neighbor] == neighbor_cube_side * neighbor_cube_side)
                    neighbor->packed_data &= ~neighbor_mask;
            }
            else {
                map[neighbor] = 1;
            }
        }
    }

    auto Chunk::find(glm::vec3 position, platform::Platform *platform) -> node::Node * {

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

    auto Chunk::find(std::function<f32(const glm::vec3 &, const u32)> &fun) -> f32 {
        const auto pos = static_cast<f32>(CHUNK_SIZE) * glm::vec3 {
                static_cast<i32>(this->chunk_idx % (RENDER_RADIUS * 2)) - RENDER_RADIUS,
                -4,
                static_cast<i32>(this->chunk_idx / (RENDER_RADIUS * 2)) - RENDER_RADIUS
        };

        auto ray_scale = std::numeric_limits<f32>::max();
        for (auto i = 0; i < this->chunk_segments.size(); ++i) {
            const auto chunk_pos = pos + glm::vec3 {
                    0.0F,
                    i * static_cast<f32>(CHUNK_SIZE),
                    0.0F
            };

            auto ret = this->chunk_segments[i].root->find(chunk_pos, fun);
            ray_scale = ret < ray_scale ? ret : ray_scale;
        }

        return ray_scale;
    }

    auto Chunk::remove(glm::vec3 position) -> void {
        auto normalized_vec = CHUNK_SEGMENT_Y_NORMALIZE(position);

        u16 x = static_cast<u8>(normalized_vec.x) & MASK_5;
        u16 y = static_cast<u8>(normalized_vec.y) & MASK_5;
        u16 z = static_cast<u8>(normalized_vec.z) & MASK_5;

        this->chunk_segments[CHUNK_SEGMENT_Y_DIFF(position)].root->removePoint(
                (x << 10) | (y << 5) | z);
    }

    auto Chunk::cull(state::State &state) const -> void {
        auto global_root = glm::vec3 {
                state.platform.get_world_root().x,
                0,
                state.platform.get_world_root().y
        };

        auto offset = static_cast<f32>(CHUNK_SIZE) * glm::vec3 {
                static_cast<i32>(this->chunk_idx % (RENDER_RADIUS * 2)) - RENDER_RADIUS,
                -4,
                static_cast<i32>(this->chunk_idx / (RENDER_RADIUS * 2)) - RENDER_RADIUS
        };

        auto pos = global_root + offset;
        auto sum = 0;
        for (u64 i = 0; i < 6; ++i)
            sum += this->mask_container[static_cast<u64>(1) << (50 + i)];

        if (!sum)
            return;

        u64 actual_size = 0;
        const auto *buffer = reinterpret_cast<chunk::chunk_renderer::ChunkRenderer &>(
                state.renderer.get_sub_renderer(rendering::renderer::CHUNK_RENDERER))
                .request_writeable_area(sum, threading::thread_pool::worker_id);

        for (u8 i = 0; i < this->chunk_segments.size(); ++i) {
            if (this->chunk_segments[i].initialized) {
                pos.y = static_cast<f32>((i - 4) * CHUNK_SIZE);
                this->chunk_segments[i].root->cull(
                        pos,
                        state.player.get_camera(),
                        buffer,
                        actual_size);
            }
        }

        ASSERT_EQ(actual_size <= sum);
        reinterpret_cast<chunk::chunk_renderer::ChunkRenderer &>(
                state.renderer.get_sub_renderer(rendering::renderer::CHUNK_RENDERER))
                .add_size_writeable_area(actual_size, threading::thread_pool::worker_id);
    }

    auto Chunk::recombine() -> void {
        for (auto &ref : this->chunk_segments)
            if (!ref.initialized) {

                // compress SVO
                ref.root->recombine();

                // count renderable faces
                this->mask_container[TOP_BIT]    += ref.root->count_mask(TOP_BIT);
                this->mask_container[BOTTOM_BIT] += ref.root->count_mask(BOTTOM_BIT);
                this->mask_container[FRONT_BIT]  += ref.root->count_mask(FRONT_BIT);
                this->mask_container[BACK_BIT]   += ref.root->count_mask(BACK_BIT);
                this->mask_container[LEFT_BIT]   += ref.root->count_mask(LEFT_BIT);
                this->mask_container[RIGHT_BIT]  += ref.root->count_mask(RIGHT_BIT);

                // indicate readiness
                ref.initialized = true;
            }
    }

    auto Chunk::update_and_render(u16 nchunk_idx,
                                  state::State &state) -> void {
        this->chunk_idx = nchunk_idx & 0xFFF;
        for (u8 i = 0; i < CHUNK_SEGMENTS; ++i) {
            if (this->chunk_segments[i].initialized) {
                this->chunk_segments[i].root->update_chunk_mask((this->chunk_idx << 4) | i);
            }
        }

        cull(state);
    }

    auto Chunk::index() const -> u16 {
        return this->chunk_idx;
    }

    auto Chunk::visible(
            const util::camera::Camera &camera,
            const platform::Platform &platform) const -> bool {
        const u64 face_mask = this->faces & static_cast<u64>(camera.get_mask());
        if (!face_mask || !this->size)
            return false;

        auto offset =
                static_cast<f32>(CHUNK_SIZE) *
                glm::vec2 {
                        static_cast<i32>(this->chunk_idx % (RENDER_RADIUS * 2)) - RENDER_RADIUS,
                        static_cast<i32>(this->chunk_idx / (RENDER_RADIUS * 2)) - RENDER_RADIUS
                };

        return camera.check_in_frustum(platform.get_world_root() + offset, CHUNK_SIZE);
    }

    auto Chunk::add_neigbor(Position position, std::shared_ptr<Chunk> neighbor) -> void {
        for (auto &[p, w] : this->neighbors)
            if (p == position && w.expired()) {
                w = std::move(neighbor);
                return;
            }

        this->neighbors.emplace_back(position, std::move(neighbor));
    }
}