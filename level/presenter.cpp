//
// Created by Luis Ruisinger on 03.06.24.
//

#include "presenter.h"

#include <ranges>

namespace core::level::presenter {
    Presenter::Presenter(
            rendering::Renderer &renderer,
            core::memory::arena_allocator::ArenaAllocator *allocator)
            : util::observer::Observer {           },
              meshes                   {           },
              platform                 { *this     },
              renderer                 { renderer  },
              allocator                { allocator },
              storage(std::thread::hardware_concurrency())
    {
        this->meshes[0] = VoxelStructure::CubeStructure{};
    }

    auto Presenter::frame(
            threading::Tasksystem<> &thread_pool,
            camera::perspective::Camera &camera) -> void {
        auto t_start = std::chrono::high_resolution_clock::now();
        this->allocator.reset();

        auto batch = this->renderer.get_batch_size();
        for (auto i = 0; i < this->storage.size(); ++i) {
            this->storage[i].clear();
            this->storage[i].push_back({
                .mem = this->allocator.allocate<VERTEX>(batch),
                .capacity = batch,
                .size = 0
            });
        }

#ifdef DEBUG
        for (auto &vec : this->storage) {
            ASSERT_EQ(vec[0].mem);
            ASSERT_EQ(vec[0].capacity == batch);
            ASSERT_EQ(vec[0].size == 0);
        }
#endif

        this->platform.frame(thread_pool, camera);
        auto t_end = std::chrono::high_resolution_clock::now();
        auto t_diff = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start);

        this->renderer.updateGlobalBase(this->platform.get_world_root());
        this->renderer.updateRenderDistance(RENDER_RADIUS);

        auto vertex_sum = 0;
        auto draw_calls = 0;

        for (auto i = 0; i < this->storage.size(); ++i) {
            for (auto j = 0; j < this->storage[i].size(); ++j) {
                if (this->storage[i][j].size > 0) {
                    ASSERT_EQ(this->storage[i][j].size <= this->renderer.get_batch_size());

                    this->renderer.updateBuffer(
                            this->storage[i][j].mem,
                            this->storage[i][j].size);
                    this->renderer.frame();

                    vertex_sum += this->storage[i][j].size;
                    ++draw_calls;
                }
            }
        }

        rendering::interface::set_render_time(t_diff);
        rendering::interface::set_draw_calls(draw_calls);
        rendering::interface::set_vertices_count(vertex_sum * sizeof(VERTEX) / sizeof(u64));
        rendering::interface::set_camera_pos(camera.getCameraPosition());
        rendering::interface::update();
        rendering::interface::render();
    }

    auto Presenter::tick(
            threading::Tasksystem<> &thread_pool,
            camera::perspective::Camera &camera) -> void  {
        this->platform.tick(thread_pool, camera);
    }

    auto Presenter::get_structure(u16 i) const -> const VoxelStructure::CubeStructure & {
        return this->meshes[i];
    }

    auto Presenter::request_writeable_area(u64 len, u64 thread_id) -> const VERTEX * {
        auto &vec = this->storage[thread_id];

        if (vec[vec.size() - 1].size + len > vec[vec.size() - 1].capacity) {
            auto batch = this->renderer.get_batch_size();

            vec.push_back({
                .mem = this->allocator.allocate<VERTEX>(batch),
                .capacity = batch,
                .size = 0
            });

            ASSERT_EQ(vec[vec.size() - 1].mem);
        }

        ASSERT_NEQ(reinterpret_cast<u64>(vec[vec.size() - 1].mem) % 32);
        ASSERT_EQ(vec[vec.size() - 1].size + len <= vec[vec.size() - 1].capacity);

        // new local thread head
        return vec[vec.size() - 1].mem + vec[vec.size() - 1].size;
    }

    auto Presenter::add_size_writeable_area(u64 len, u64 thread_id) -> void {
        auto &vec = this->storage[thread_id];
        vec[vec.size() - 1].size += len;

        ASSERT_EQ(vec[vec.size() - 1].size <= vec[vec.size() - 1].capacity);
    }
}