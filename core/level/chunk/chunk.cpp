// Created by Luis Ruisinger on 18.02.24.
//

#ifdef __AVX2__
#define GLM_FORCE_AVX
#endif

#include "chunk.h"
#include "chunk_renderer.h"
#include "generation/generation.h"

#include "../chunk_data_structure/voxel_data_layout.h"

#include "../util/assert.h"
#include "../util/player.h"

namespace core::level::chunk {
    using rendering::renderer::RenderType;

    auto OcclusionMap::operator[](u64 mask) -> std::unordered_map<node::Node *, u32> & {
        switch (mask) {
            case TOP_BIT   : return this->map[0];
            case BOTTOM_BIT: return this->map[1];
            case FRONT_BIT : return this->map[2];
            case BACK_BIT  : return this->map[3];
            case LEFT_BIT  : return this->map[4];
            case RIGHT_BIT : return this->map[5];
        }

#if defined(__GNUC__)
        __builtin_unreachable();
#elif defined(_MSC_VER)
        __assume(false);
#endif
    }

    Chunk::Chunk(u16 chunk_idx)
            : chunk_idx { static_cast<u16>(chunk_idx & 0xFFF) }
    {
        this->chunk_pos = {
                static_cast<int>((this->chunk_idx % (RENDER_RADIUS * 2)) - RENDER_RADIUS),
                0,
                static_cast<int>((this->chunk_idx / (RENDER_RADIUS * 2)) - RENDER_RADIUS)
        };

        this->chunk_pos *= static_cast<f32>(CHUNK_SIZE);

        for (u8 i = 0; i < CHUNK_SEGMENTS; ++i)
            this->chunk_segments.emplace_back(i);
    }

    auto Chunk::generate(glm::ivec2 root) -> void {
        auto offset = root + glm::ivec2(this->chunk_pos.x, this->chunk_pos.z);
        generation::generation::Generator::generate(*this, offset);

        for (size_t i = 0; i < chunk_segments.size(); ++i) {
            this->faces |= this->chunk_segments[i]
                    .voxel_root->updateFaceMask((this->chunk_idx << 4) | i);

            this->faces |= this->chunk_segments[i]
                    .water_root->updateFaceMask((this->chunk_idx << 4) | i);
        }
    }

    auto Chunk::find(glm::ivec3 position) -> node::Node * {

        // inter-chunk-finding
        if (position.x >= CHUNK_SIZE) {
            for (const auto &[p, w] : this->neighbors)
                if (p == Position::FRONT) {
                    if (auto ptr = w.lock()) {
                        position.x -= CHUNK_SIZE;
                        ASSERT_EQ(ptr.get());
                        return ptr->find(position);
                    }
                }
        }
        else if (position.x < 0.0F) {
            for (const auto &[p, w] : this->neighbors)
                if (p == Position::BACK) {
                    if (auto ptr = w.lock()) {
                        position.x += CHUNK_SIZE;
                        ASSERT_EQ(ptr.get());
                        return ptr->find(position);
                    }
                }
        }
        else if (position.z >= CHUNK_SIZE) {
            for (const auto& [p, w] : this->neighbors)
                if (p == Position::RIGHT) {
                    if (auto ptr = w.lock()) {
                        position.z -= CHUNK_SIZE;
                        ASSERT_EQ(ptr.get());
                        return ptr->find(position);
                    }
                }
        }
        else if (position.z < 0.0F) {
            for (const auto& [p, w] : this->neighbors)
                if (p == Position::LEFT) {
                    if (auto ptr = w.lock()) {
                        position.z += CHUNK_SIZE;
                        ASSERT_EQ(ptr.get());
                        return ptr->find(position);
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

            u32 mask = (x << 13) | (y << 8) | (z << 3) | MASK_3;
            auto *candidate = segment.voxel_root->find(mask);

            return candidate ? candidate : segment.water_root->find(mask);
        }

        return nullptr;
    }

    template <>
    auto Chunk::insert<RenderType::CHUNK_RENDERER>(
            const glm::ivec3 position,
            u16 voxel_ID,
            bool recombine) -> void {
        auto normalized_vec = CHUNK_SEGMENT_Y_NORMALIZE(position);
        auto &segment = this->chunk_segments[CHUNK_SEGMENT_Y_DIFF(position)];

        u64 x = static_cast<u8>(normalized_vec.x) & MASK_5;
        u64 y = static_cast<u8>(normalized_vec.y) & MASK_5;
        u64 z = static_cast<u8>(normalized_vec.z) & MASK_5;

        // setting coordinates
        u32 packed_data_highp = (x << 13) | (y <<  8) | (z <<  3) | MASK_3;

        // 12 highest bit set to the index of the chunk inside chunk managing array
        u32 packed_data_lowp =
                (this->chunk_idx << 20) |
                (segment.segment_idx << 16) |
                (voxel_ID & 0x1FF);

        auto *node = segment.voxel_root->addPoint(
                (static_cast<u64>(packed_data_highp) << SHIFT_HIGH) | packed_data_lowp);

        f32 offset = 1 << ((node->packed_data >> SHIFT_HIGH) & MASK_3);

        // occlusion culling
        update_occlusion(node, find(position - glm::ivec3 {1, 0, 0}), LEFT_BIT, RIGHT_BIT);
        update_occlusion(node, find(position + glm::ivec3 {1, 0, 0}), RIGHT_BIT, LEFT_BIT);
        update_occlusion(node, find(position - glm::ivec3 {0, 1, 0}), BOTTOM_BIT, TOP_BIT);
        update_occlusion(node, find(position + glm::ivec3 {0, 1, 0}), TOP_BIT, BOTTOM_BIT);
        update_occlusion(node, find(position - glm::ivec3 {0, 0, 1}), BACK_BIT, FRONT_BIT);
        update_occlusion(node, find(position + glm::ivec3 {0, 0, 1}), FRONT_BIT, BACK_BIT);

        // recombining voxels
        if (recombine) {
            segment.voxel_root->recombine();
            segment.chunk_modified = true;
        }
    }

    template <>
    auto Chunk::insert<RenderType::WATER_RENDERER>(
            const glm::ivec3 position,
            u16 voxel_ID,
            bool recombine) -> void {
        auto normalized_vec = CHUNK_SEGMENT_Y_NORMALIZE(position);
        auto &segment = this->chunk_segments[CHUNK_SEGMENT_Y_DIFF(position)];

        u64 x = static_cast<u8>(normalized_vec.x) & MASK_5;
        u64 y = static_cast<u8>(normalized_vec.y) & MASK_5;
        u64 z = static_cast<u8>(normalized_vec.z) & MASK_5;

        // setting coordinates
        u32 packed_data_highp = (x << 13) | (y <<  8) | (z <<  3) | MASK_3;

        // 12 highest bit set to the index of the chunk inside chunk managing array
        u32 packed_data_lowp =
                (this->chunk_idx << 20) |
                (segment.segment_idx << 16) |
                (voxel_ID & 0x1FF);

        auto *node = segment.water_root->addPoint(
                (static_cast<u64>(packed_data_highp) << SHIFT_HIGH) | packed_data_lowp);

        f32 offset = 1 << ((node->packed_data >> SHIFT_HIGH) & MASK_3);

        // occlusion culling
        update_occlusion(node, find(position - glm::ivec3 {1, 0, 0}), LEFT_BIT, RIGHT_BIT);
        update_occlusion(node, find(position + glm::ivec3 {1, 0, 0}), RIGHT_BIT, LEFT_BIT);
        update_occlusion(node, find(position - glm::ivec3 {0, 1, 0}), BOTTOM_BIT, TOP_BIT);
        update_occlusion(node, find(position + glm::ivec3 {0, 1, 0}), TOP_BIT, BOTTOM_BIT);
        update_occlusion(node, find(position - glm::ivec3 {0, 0, 1}), BACK_BIT, FRONT_BIT);
        update_occlusion(node, find(position + glm::ivec3 {0, 0, 1}), FRONT_BIT, BACK_BIT);

        // recombining voxels
        if (recombine) {
            segment.water_root->recombine();
            segment.chunk_modified = true;
        }
    }

    inline
    auto Chunk::update_occlusion(
            node::Node *current,
            node::Node *neighbor,
            u64 current_mask,
            u64 neighbor_mask) -> void {
        if (!neighbor)
            return;

        const auto current_id = current->packed_data & 0x1FF;
        const auto neighbor_id = neighbor->packed_data & 0x1FF;
        const auto &current_voxel_config = tiles::tile_manager::tile_manager[current_id];
        const auto &neighbor_voxel_config = tiles::tile_manager::tile_manager[neighbor_id];

        // every voxel we insert has BASE_SIZE and thus is guaranteed
        // to be blocked if a neighbor exists for this face

        if (neighbor_voxel_config.can_cull(current_voxel_config))
            current->packed_data &= ~current_mask;

        if (!current_voxel_config.can_cull(neighbor_voxel_config))
            return;

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


    auto Chunk::find(std::function<f32(const glm::vec3 &,const u32)> &fun) -> f32 {
        auto ray_scale = std::numeric_limits<f32>::max();

        for (auto i = 0; i < this->chunk_segments.size(); ++i) {
            const auto offset = glm::ivec3(0, (i - 4) * CHUNK_SIZE, 0);
            const auto position = this->chunk_pos + offset;
            const auto ret = this->chunk_segments[i].voxel_root->find(position, fun);

            ray_scale = ret < ray_scale ? ret : ray_scale;
        }

        return ray_scale;
    }
    
    template<> 
    auto Chunk::remove<RenderType::CHUNK_RENDERER>(
            glm::ivec3 position) -> void {
        auto normalized_vec = CHUNK_SEGMENT_Y_NORMALIZE(position);

        u16 x = static_cast<u8>(normalized_vec.x) & MASK_5;
        u16 y = static_cast<u8>(normalized_vec.y) & MASK_5;
        u16 z = static_cast<u8>(normalized_vec.z) & MASK_5;

        const u16 compressed_pos = (x << 10) | (y << 5) | z;
        this->chunk_segments[CHUNK_SEGMENT_Y_DIFF(position)].voxel_root->removePoint(compressed_pos);
    }

    template<>
    auto Chunk::remove<RenderType::WATER_RENDERER>(
            glm::ivec3 position) -> void {
        auto normalized_vec = CHUNK_SEGMENT_Y_NORMALIZE(position);

        u16 x = static_cast<u8>(normalized_vec.x) & MASK_5;
        u16 y = static_cast<u8>(normalized_vec.y) & MASK_5;
        u16 z = static_cast<u8>(normalized_vec.z) & MASK_5;

        const u16 compressed_pos = (x << 10) | (y << 5) | z;
        this->chunk_segments[CHUNK_SEGMENT_Y_DIFF(position)].water_root->removePoint(compressed_pos);
    }

    auto Chunk::cull(state::State &state) const -> void {
        auto global_root = state.platform.get_world_root();
        auto pos = glm::ivec3(global_root.x, 0, global_root.y) + this->chunk_pos;

        if (this->voxel_size) {
            u64 actual_size = 0;
            auto *buffer = reinterpret_cast<chunk::chunk_renderer::ChunkRenderer &>(
                    state.renderer
                        .get_sub_renderer(rendering::renderer::CHUNK_RENDERER))
                        .request_writeable_area(this->voxel_size, threading::thread_pool::worker_id);

            for (u8 i = 0; i < this->chunk_segments.size(); ++i) {
                if (this->chunk_segments[i].initialized) {
                    pos.y = (i - 4) * CHUNK_SIZE;
                    this->chunk_segments[i].voxel_root->cull(
                            pos, state.player.get_camera(), buffer, actual_size);
                }
            }

            ASSERT_EQ(actual_size <= this->water_size);
            reinterpret_cast<chunk::chunk_renderer::ChunkRenderer &>(
                    state.renderer
                        .get_sub_renderer(rendering::renderer::CHUNK_RENDERER))
                        .add_size_writeable_area(actual_size, threading::thread_pool::worker_id);
        }

        if (this->water_size) {
            u64 actual_size = 0;
            auto  *buffer = reinterpret_cast<chunk::chunk_renderer::ChunkRenderer &>(
                    state.renderer
                        .get_sub_renderer(rendering::renderer::WATER_RENDERER))
                        .request_writeable_area(this->water_size, threading::thread_pool::worker_id);

            for (u8 i = 0; i < this->chunk_segments.size(); ++i) {
                if (this->chunk_segments[i].initialized) {
                    pos.y = static_cast<f32>((i - 4) * CHUNK_SIZE);
                    this->chunk_segments[i].water_root->cull(
                            pos, state.player.get_camera(), buffer, actual_size);
                }
            }

            ASSERT_EQ(actual_size <= this->water_size);
            reinterpret_cast<chunk::chunk_renderer::ChunkRenderer &>(
                    state.renderer
                        .get_sub_renderer(rendering::renderer::WATER_RENDERER))
                        .add_size_writeable_area(actual_size, threading::thread_pool::worker_id);
        }
    }

    auto Chunk::recombine() -> void {
        for (auto &ref : this->chunk_segments) {
            if (!ref.initialized) {

                // compress SVO
                ref.voxel_root->recombine();
                this->voxel_size +=
                        ref.voxel_root->count_mask(TOP_BIT) +
                        ref.voxel_root->count_mask(BOTTOM_BIT) +
                        ref.voxel_root->count_mask(FRONT_BIT) +
                        ref.voxel_root->count_mask(BACK_BIT) +
                        ref.voxel_root->count_mask(LEFT_BIT) +
                        ref.voxel_root->count_mask(RIGHT_BIT);

                // compress SVO
                ref.water_root->recombine();
                this->water_size += ref.water_root->count_mask(TOP_BIT);

                // indicate readiness
                ref.initialized = true;
            }
        }
    }

    auto Chunk::update_and_render(u16 nchunk_idx, state::State &state) -> void {
        this->chunk_idx = nchunk_idx & 0xFFF;
        this->chunk_pos = glm::ivec3 {
                (this->chunk_idx % (RENDER_RADIUS * 2)) - RENDER_RADIUS,
                0,
                (this->chunk_idx / (RENDER_RADIUS * 2)) - RENDER_RADIUS
        };
        this->chunk_pos *= static_cast<f32>(CHUNK_SIZE);

        for (u8 i = 0; i < CHUNK_SEGMENTS; ++i) {
            if (this->chunk_segments[i].initialized) {
                this->chunk_segments[i].voxel_root->update_chunk_mask((this->chunk_idx << 4) | i);
                this->chunk_segments[i].water_root->update_chunk_mask((this->chunk_idx << 4) | i);
            }
        }

        cull(state);
    }

    auto Chunk::index() const -> u16 {
        return this->chunk_idx;
    }

    auto Chunk::visible(
            const util::camera::Camera &camera, const glm::ivec2 &world_offset) const -> bool {
        const u64 face_mask = this->faces & static_cast<u64>(camera.get_mask());
        if (!face_mask || !(this->voxel_size + this->water_size))
            return false;

        auto offset = glm::ivec2(this->chunk_pos.x, this->chunk_pos.z);
        return camera.check_in_frustum(world_offset + offset, CHUNK_SIZE);
    }

    auto Chunk::add_neigbor(Position position, std::shared_ptr<Chunk> neighbor) -> void {
        for (auto &[p, w] : this->neighbors) {
            if (p == position && w.expired()) {
                w = std::move(neighbor);
                return;
            }
        }

        this->neighbors.emplace_back(position, std::move(neighbor));
    }
}