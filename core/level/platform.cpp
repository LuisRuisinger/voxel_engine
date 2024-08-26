//
// Created by Luis Ruisinger on 18.02.24.
//

#include <ranges>

#include "platform.h"
#include "../rendering/interface.h"
#include "../../util/assert.h"

#define INDEX(_x, _z) \
    ((((_x) + RENDER_RADIUS)) + (((_z) + RENDER_RADIUS) * (2 * RENDER_RADIUS)))

#define POSITION(_i, _r) \
    (glm::vec2 { (((_i) % ((_r) * 2)) - (_r)), \
                 (((_i) / ((_r) * 2)) - (_r))})

#define DISTANCE_2D(_p1, _p2) \
    (std::hypot((_p1).x - (_p2).x, (_p1).y - (_p2).y))

#define LOAD_THRESHOLD(_p1, _p2) \
    (DISTANCE_2D((_p1), (_p2)) >= CHUNK_SIZE * 2)

namespace core::level::platform {
    Platform::Platform(presenter::Presenter &_presenter)
        : presenter { _presenter }
    {}

    /**
     * @brief Update platform if needed. Load new chunks if threshold is hit.
     * @param thread_pool Pool to offload tasks.
     * @param camera Current active camera.
     */
    auto Platform::tick(threading::thread_pool::Tasksystem<> &thread_pool,
                        util::camera::Camera &camera) -> void {
        const auto &cameraPos = camera.get_position();
        const auto new_root = glm::vec2{
            static_cast<i32>(std::lround(static_cast<i32>(cameraPos.x / CHUNK_SIZE)) * CHUNK_SIZE),
            static_cast<i32>(std::lround(static_cast<i32>(cameraPos.z / CHUNK_SIZE)) * CHUNK_SIZE)
        };

        ASSERT_NEQ(static_cast<i32>(new_root.x) % CHUNK_SIZE,
                  "global platform roots must be multiple of 32");
        ASSERT_NEQ(static_cast<i32>(new_root.y) % CHUNK_SIZE,
                  "global platform roots must be multiple of 32");

        // threshold to render new chunks is double the chunk size
        if (!this->queue_ready &&
            (LOAD_THRESHOLD(this->current_root, new_root) || !this->platform_ready)) {

            DEBUG_LOG(new_root);
            load_chunks(thread_pool, new_root)
                .swap_chunks(new_root)
                .unload_chunks(thread_pool);
        }
    }

    /**
     * @brief Unload chunks whose shared count is 1, meaning they are no in use in this cycle.
     * @param thread_pool Threadpool to parallel destroy unused chunks.
     */
    auto Platform::unload_chunks(threading::thread_pool::Tasksystem<> &thread_pool) -> Platform & {
        static auto destroy = [](std::shared_ptr<chunk::Chunk> ptr) -> void {
            ASSERT_EQ(ptr.get());
        };

        DEBUG_LOG("Unloading chunks");

        std::vector<decltype(this->chunks)::key_type> to_erase;
        for (auto &[_, k] : this->queued_chunks) {

            bool active = false;
            for (const auto &[__, v] : this->active_chunks) {
                if (k == v) {
                    active = true;
                    break;
                }
            }

            if (!active) {
                thread_pool.enqueue_detach(destroy, std::move(this->chunks[k]));
                to_erase.push_back(k);
            }
        }

        for (auto &k : to_erase)
            this->chunks.erase(k);

        this->queued_chunks.clear();
        DEBUG_LOG("Finished unloading chunks");

        return *this;
    }

    auto Platform::init_chunk_neighbours(i32 i,
                                         i32 j,
                                         chunk::Position p1,
                                         chunk::Position p2) -> void {
        if (j > -1 && j < MAX_RENDER_VOLUME &&
            this->queued_chunks.contains(j)) {
            this->queued_chunks[i]->add_neigbor(p1, this->chunks[this->queued_chunks[j]]);
            this->queued_chunks[j]->add_neigbor(p2, this->chunks[this->queued_chunks[i]]);
        }
    }

#define INIT_NEIGHBOR(_x, _z) ({                                                    \
        this->init_chunk_neighbours(INDEX((_x), (_z)), INDEX((_x) - 1, (_z)),       \
                                    chunk::Position::BACK, chunk::Position::FRONT); \
        this->init_chunk_neighbours(INDEX((_x), (_z)), INDEX((_x) + 1, (_z)),       \
                                    chunk::Position::FRONT, chunk::Position::BACK); \
        this->init_chunk_neighbours(INDEX((_x), (_z)), INDEX((_x), (_z) - 1),       \
                                    chunk::Position::LEFT, chunk::Position::RIGHT); \
        this->init_chunk_neighbours(INDEX((_x), (_z)), INDEX((_x), (_z) + 1),       \
                                    chunk::Position::RIGHT, chunk::Position::LEFT); \
    })

    /**
     * @brief Load new chunks and share ownership of currently loaded chunks that are still
     *        visible in the new region.
     * @param thread_pool Threadpool to parallel generate new chunks.
     * @param new_root    The center of the new region.
     */
    auto Platform::load_chunks(threading::thread_pool::Tasksystem<> &thread_pool,
                               glm::vec2 new_root) -> Platform & {
        static auto generate = [](chunk::Chunk *ptr, Platform *platform) -> void {
            ASSERT_EQ(ptr);
            ptr->generate(platform);
        };

        static auto compress = [](chunk::Chunk *ptr) -> void {
            ASSERT_EQ(ptr);
            ptr->recombine();
        };

        std::vector<u64> generated {};
        for (i32 x = -RENDER_RADIUS; x < RENDER_RADIUS; ++x) {
            for (i32 z = -RENDER_RADIUS; z < RENDER_RADIUS; ++z) {
                if (DISTANCE_2D(glm::vec2(-0.5), glm::vec2(x, z)) < RENDER_RADIUS) {

                    // position of the (x, z) chunk in the coordinate space of
                    // the old root
                    auto old_pos =
                            (glm::vec2(x, z) * static_cast<f32>(CHUNK_SIZE) +
                            new_root - current_root) / static_cast<f32>(CHUNK_SIZE);

                    if (DISTANCE_2D(glm::vec2(-0.5), old_pos) < RENDER_RADIUS &&
                        this->platform_ready) [[likely]] {

                        this->queued_chunks[INDEX(x, z)] =
                                this->active_chunks[INDEX(old_pos.x, old_pos.y)];
                        INIT_NEIGHBOR(x, z);
                    }
                    else {
                        auto ptr = std::make_shared<chunk::Chunk>(INDEX(x, z));
                        auto chunk = ptr.get();

                        this->chunks[chunk] = std::move(ptr);
                        this->queued_chunks[INDEX(x, z)] = chunk;
                        INIT_NEIGHBOR(x, z);

                        // generate new chunk
                        thread_pool.enqueue_detach(generate, chunk, this);
                        generated.push_back(INDEX(x, z));
                    }

                }
            }
        }

        thread_pool.wait_for_tasks();

        for (auto i : generated)
            thread_pool.enqueue_detach(
                    compress,
                    this->queued_chunks[i]);

        thread_pool.wait_for_tasks();
        DEBUG_LOG("Finished chunk initialization");
        return *this;
    }

    /**
    * @brief Sliding window principle to swap active chunks with the new region.
    * @param new_root The center of the new region.
    */
    auto Platform::swap_chunks(glm::vec2 new_root) -> Platform & {
        {
            std::unique_lock lock { this->mutex };
            std::swap(this->active_chunks, this->queued_chunks);

            this->active_chunks_vec.clear();
            for (const auto &[k, v] : this->active_chunks)
                this->active_chunks_vec.emplace_back(k, v);

            this->current_root = new_root;
            this->queue_ready = true;
            this->platform_ready = true;
        }

        DEBUG_LOG("Chunks swapped");
        return *this;
    }

    /**
     * @brief Extract visible mesh for current frame.
     * @param thread_pool Parallel traversal of single chunks.
     * @param camera      Active camera for this frame.
     */
    auto Platform::frame(threading::thread_pool::Tasksystem<> &thread_pool,
                         util::camera::Camera &camera) -> void {
        static auto update_render_fun = [](u16 idx,
                                           chunk::Chunk *ptr,
                                           util::camera::Camera &camera,
                                           Platform &platform) -> void {
            ptr->update_and_render(idx, camera, platform);
        };

        static auto render_fun = [](chunk::Chunk *ptr,
                                    util::camera::Camera &camera,
                                    Platform &platform) -> void {
            ptr->cull(camera, platform);
        };

        // check if new chunks need to be added to the active pool
        // try lock to ensure no amount of frame freeze happens
        std::unique_lock lock { this->mutex };


        if (this->queue_ready) {
            for (const auto& [k, v] : this->active_chunks_vec) {
                if (v) {
                    thread_pool.enqueue_detach(update_render_fun,
                                               k,
                                               v,
                                               std::ref(camera),
                                               std::ref(*this));
                }
            }

            this->queue_ready = false;
        }
        else [[likely]] {
            for (const auto& [_, v] : this->active_chunks_vec) {
                if (v) {
                    thread_pool.enqueue_detach(render_fun,
                                               v,
                                               std::ref(camera),
                                               std::ref(*this));
                }
            }
        }

        thread_pool.wait_for_tasks();
    }

    /** @brief Get current root position of the platform. */
    auto Platform::get_world_root() const -> glm::vec2 {
        return this->current_root;
    }

    /** @brief Get handle to presenter handling the platform. */
    auto Platform::get_presenter() const -> presenter::Presenter & {
        return this->presenter;
    }
}