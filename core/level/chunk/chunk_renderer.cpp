//
// Created by Luis Ruisinger on 26.08.24.
//

#include "chunk_renderer.h"

namespace core::level::chunk::chunk_renderer {
    ChunkRenderer::ChunkRenderer(arena_allocator::ArenaAllocator *allocator)
            : meshes                   {           },
              allocator                { allocator },
              storage(std::thread::hardware_concurrency()) {
        // TODO: later init voxel meshes here
        this->meshes[0] = model::voxel::CubeStructure{};
    }

    auto ChunkRenderer::init_shader() -> void {
        auto res = this->shader.init("vertex_shader.glsl", "fragment_shader.glsl");
        if (res.isErr()) {
            LOG(res.unwrapErr());
            std::exit(EXIT_FAILURE);
        }

        // setting up uniforms
        this->shader.registerUniformLocation("view");
        this->shader.registerUniformLocation("projection");
        this->shader.registerUniformLocation("worldbase");
        this->shader.registerUniformLocation("render_radius");

        // thin wrapper over VAO layouts
        this->layout
                .begin(sizeof(u64))
                .add(1, util::renderable::Type::U_INT, nullptr)
                .add(1, util::renderable::Type::U_INT, sizeof(u32))
                .end();
    }

    auto ChunkRenderer::prepare_frame(state::State &state) -> void {
        this->allocator.reset();

        auto _batch = batch(sizeof(VERTEX));
        for (auto i = 0; i < this->storage.size(); ++i) {
            this->storage[i].clear();
            this->storage[i].push_back({
                .mem = this->allocator.allocate<VERTEX>(_batch),
                .capacity = _batch,
                .size = 0
            });
        }

#ifdef DEBUG
        for (auto &vec : this->storage) {
            ASSERT_EQ(vec[0].mem);
            ASSERT_EQ(vec[0].capacity == _batch);
            ASSERT_EQ(vec[0].size == 0);
        }
#endif
    }

    auto ChunkRenderer::frame(state::State &state) -> void {
        auto view = state.player
                .get_camera()
                .get_view_matrix();

        auto projection = state.player
                .get_camera()
                .get_projection_matrix();

        this->shader["view"] = view;
        this->shader["projection"] = projection;
        this->shader["worldbase"] = state.platform.get_world_root();
        this->shader["render_radius"] = RENDER_RADIUS;
        this->shader.upload_uniforms();

        auto vertex_sum = 0;
        auto draw_calls = 0;
        auto _batch = batch(sizeof(VERTEX));

        for (auto i = 0; i < this->storage.size(); ++i) {
            for (auto j = 0; j < this->storage[i].size() - 1; ++j) {

                auto size = this->storage[i][j].size;
                if (size == 0)
                    continue;

                update_buffer(this->storage[i][j].mem, sizeof(VERTEX), size);
                draw();
                vertex_sum += size;
                ++draw_calls;
            }
        }

        u64 offset = 0;
        auto i = 0;

        // reduces overhead in draw calls
        while (i < this->storage.size()) {
            auto able_to_take = _batch;

            while (able_to_take > 0 && i < this->storage.size()) {
                auto& current_storage = this->storage[i].back();
                auto size = current_storage.size;

                if (size == 0) {
                    ++i;
                    offset = 0;
                    continue;
                }

                auto remaining_size = size - offset;
                auto to_take = std::min<size_t>(remaining_size, able_to_take);
                update_buffer(
                        current_storage.mem + offset,
                        sizeof(VERTEX),
                        to_take
                );

                vertex_sum += to_take;
                able_to_take -= to_take;
                offset += to_take;

                if (offset == size) {
                    offset = 0;
                    ++i;
                }
            }

            draw();
            ++draw_calls;
        }

        rendering::interface::set_draw_calls(draw_calls);
        rendering::interface::set_vertices_count(vertex_sum * sizeof(VERTEX) / sizeof(u64));
        rendering::interface::set_camera_pos(state.player.get_camera().get_position());
        rendering::interface::update();
        rendering::interface::render();
    }

    auto ChunkRenderer::request_writeable_area(u64 len, u64 thread_id) -> const VERTEX * {
        auto &vec = this->storage[thread_id];

        if (vec[vec.size() - 1].size + len > vec[vec.size() - 1].capacity) {
            auto _batch = batch(sizeof(VERTEX));

            vec.push_back({
                .mem = this->allocator.allocate<VERTEX>(_batch),
                .capacity = _batch,
                .size = 0
            });

            ASSERT_EQ(vec[vec.size() - 1].mem);
        }

        ASSERT_NEQ(reinterpret_cast<u64>(vec[vec.size() - 1].mem) % 32);
        ASSERT_EQ(vec[vec.size() - 1].size + len <= vec[vec.size() - 1].capacity);

        // new local thread head
        return vec[vec.size() - 1].mem + vec[vec.size() - 1].size;
    }

    auto ChunkRenderer::add_size_writeable_area(u64 len, u64 thread_id) -> void {
        auto &vec = this->storage[thread_id];
        vec[vec.size() - 1].size += len;

        ASSERT_EQ(vec[vec.size() - 1].size <= vec[vec.size() - 1].capacity);
    }

    auto ChunkRenderer::operator[](size_t i) -> model::voxel::CubeStructure & {
        return this->meshes[i];
    }
}