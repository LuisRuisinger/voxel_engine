//
// Created by Luis Ruisinger on 18.02.24.
//

#include <ranges>

#include "platform.h"
#include "../rendering/interface.h"
#include "../../util/assert.h"
#include "../../util/player.h"

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

    /**
     * @brief Update platform if needed. Load new chunks if threshold is hit.
     * @param thread_pool Pool to offload tasks.
     * @param camera Current active camera.
     */
    auto Platform::tick(state::State &state) -> void {
        const auto &cameraPos = state.player
                .get_camera()
                .get_position();

        const auto new_root_candidate = glm::vec2{
                std::lround(static_cast<i32>(cameraPos.x / CHUNK_SIZE)) * CHUNK_SIZE,
                std::lround(static_cast<i32>(cameraPos.z / CHUNK_SIZE)) * CHUNK_SIZE
        };

        switch (this->level_state) {

            // the initial state of a platform - nothing is loaded
            case INIT: {
                this->new_root = new_root_candidate;
                load_chunks(state.chunk_tick_pool);
                this->level_state = LOADING;

                break;
            }

            // no threshold has been passed
            case IDLE: {
                if (this->queue_ready || !LOAD_THRESHOLD(this->current_root, new_root_candidate))
                    break;

                if (!state.chunk_tick_pool.no_tasks())
                    break;

                this->new_root = new_root_candidate;
                load_chunks(state.chunk_tick_pool);
                this->level_state = LOADING;

                break;
            };

            // loading and initializing new chunks
            case LOADING: {
                if (!state.chunk_tick_pool.no_tasks())
                    break;

                compress_chunks(state.chunk_tick_pool);
                this->level_state = COMPRESSING;

                break;
            };

            // compressing the SVO's inside the chunks
            case COMPRESSING: {
                if (!state.chunk_tick_pool.no_tasks())
                    break;

                swap_chunks();
                this->level_state = SWAPPING;
                break;
            }

            // sliding window swap of active renderable chunks
            case SWAPPING: {
                unload_chunks(state.chunk_tick_pool);
                this->level_state = UNLOADING;
                break;
            };

            // unloading inactive, old chunks
            case UNLOADING: {
                if (!state.chunk_tick_pool.no_tasks())
                    break;

                this->level_state = IDLE;
                break;
            };
        }
    }

    /**
     * @brief Unload chunks whose shared count is 1, meaning they are no in use in this cycle.
     * @param thread_pool Threadpool to parallel destroy unused chunks.
     */
    auto Platform::unload_chunks(threading::thread_pool::Tasksystem<> &thread_pool) -> void {
        static auto destroy = [](std::shared_ptr<chunk::Chunk> ptr) -> void {
            // ASSERT_EQ(ptr.get());
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
    }

    auto Platform::init_neighbors(i32 x, i32 z) -> void {
        auto init_chunk_neighbours = [this](i32 i, i32 j, chunk::Position p1, chunk::Position p2) {
            if ((j > -1 && j < MAX_RENDER_VOLUME) && this->queued_chunks.contains(j)) {
                this->queued_chunks[i]->add_neigbor(p1, this->chunks[this->queued_chunks[j]]);
                this->queued_chunks[j]->add_neigbor(p2, this->chunks[this->queued_chunks[i]]);
            }
        };

        if (DISTANCE_2D(glm::vec2(-0.5), glm::vec2(x - 1, z)) < RENDER_RADIUS) {
            init_chunk_neighbours(
                    INDEX(x, z), INDEX(x - 1, z),
                    chunk::Position::BACK, chunk::Position::FRONT);
        }


        if (DISTANCE_2D(glm::vec2(-0.5), glm::vec2(x + 1, z)) < RENDER_RADIUS) {
            init_chunk_neighbours(
                    INDEX(x, z), INDEX(x + 1, z),
                    chunk::Position::FRONT, chunk::Position::BACK);
        }

        if (DISTANCE_2D(glm::vec2(-0.5), glm::vec2(x, z - 1)) < RENDER_RADIUS) {
            init_chunk_neighbours(
                    INDEX(x, z), INDEX(x, z - 1),
                    chunk::Position::LEFT, chunk::Position::RIGHT);
        }

        if (DISTANCE_2D(glm::vec2(-0.5), glm::vec2(x, z + 1)) < RENDER_RADIUS) {
            init_chunk_neighbours(
                    INDEX(x, z), INDEX(x, z + 1),
                    chunk::Position::RIGHT, chunk::Position::LEFT);
        }
    }

    /**
     * @brief Load new chunks and share ownership of currently loaded chunks that are still
     *        visible in the new region.
     * @param thread_pool Threadpool to parallel generate new chunks.
     * @param new_root    The center of the new region.
     */
    auto Platform::load_chunks(threading::thread_pool::Tasksystem<> &thread_pool) -> void {
        static auto generate = [](chunk::Chunk *ptr, glm::vec2 root) -> void {
            ASSERT_EQ(ptr);
            ptr->generate(root);
        };

        for (i32 x = -RENDER_RADIUS; x < RENDER_RADIUS; ++x) {
            for (i32 z = -RENDER_RADIUS; z < RENDER_RADIUS; ++z) {
                if (DISTANCE_2D(glm::vec2(-0.5), glm::vec2(x, z)) < RENDER_RADIUS) {

                    // position of the (x, z) chunk in the coordinate space of
                    // the old root
                    auto old_pos =
                            (glm::vec2(x, z) * static_cast<f32>(CHUNK_SIZE) +
                            this->new_root - this->current_root) / static_cast<f32>(CHUNK_SIZE);

                    if (DISTANCE_2D(glm::vec2(-0.5), old_pos) < RENDER_RADIUS &&
                        this->level_state == IDLE) {

                        this->queued_chunks[INDEX(x, z)] =
                                this->active_chunks[INDEX(old_pos.x, old_pos.y)];
                        init_neighbors(x, z);
                    }
                    else {
                        auto ptr = std::make_shared<chunk::Chunk>(INDEX(x, z));
                        auto chunk = ptr.get();

                        this->chunks[chunk] = std::move(ptr);
                        this->queued_chunks[INDEX(x, z)] = chunk;
                        init_neighbors(x, z);

                        // generate new chunk
                        thread_pool.enqueue_detach(generate, chunk, this->new_root);
                    }

                }
            }
        }
    }

    auto Platform::compress_chunks(threading::thread_pool::Tasksystem<> &thread_pool) -> void {
        static auto compress = [](chunk::Chunk *ptr) -> void {
            ASSERT_EQ(ptr);
            ptr->recombine();
        };

        for (auto &[k ,v] : this->queued_chunks) {
            if (k == v->index())
                thread_pool.enqueue_detach(compress, v);
        }
    }

    /**  @brief Sliding window principle to swap active chunks with the new region. */
    auto Platform::swap_chunks() -> void {
        {
            std::unique_lock lock { this->mutex };
            std::swap(this->active_chunks, this->queued_chunks);

            this->active_chunks_vec.clear();
            for (const auto &[k, v] : this->active_chunks)
                this->active_chunks_vec.emplace_back(k, v);

            this->current_root = this->new_root;
            this->queue_ready = true;
        }
    }

    /**
     * @brief Extract visible mesh for current frame.
     * @param thread_pool Parallel traversal of single chunks.
     * @param camera      Active camera for this frame.
     */
    auto Platform::update(state::State &state) -> void {
        static auto update_render_fun = [](
                u16 idx,
                chunk::Chunk *ptr,
                state::State &state) -> void {
            ptr->update_and_render(idx, state);
        };

        static auto render_fun = [](
                chunk::Chunk *ptr,
                state::State &state) -> void {
            ptr->cull(state);
        };

        // check if new chunks need to be added to the active pool
        // try lock to ensure no amount of frame freeze happens
        std::unique_lock lock { this->mutex };

        if (this->queue_ready) {
            for (const auto& [k, v] : this->active_chunks_vec) {
                if (v) {
                    state.render_pool.enqueue_detach(
                            update_render_fun,
                            k,
                            v,
                            state);
                }
            }

            this->queue_ready = false;
        }
        else [[likely]] {
            for (const auto& [_, v] : this->active_chunks_vec) {
                if (v) {
                    state.render_pool.enqueue_detach(
                            render_fun,
                            v,
                            state);
                }
            }
        }

        state.render_pool.wait_for_tasks();
    }

    /** @brief Get current root position of the platform. */
    auto Platform::get_world_root() const -> glm::vec2 {
        return this->current_root;
    }

    auto Platform::get_nearest_chunks(const glm::vec3 &pos)
        -> std::optional<std::array<chunk::Chunk *, 4>> {
        std::unique_lock lock { this->mutex };

        auto root = glm::vec2 {
                static_cast<i32>(pos.x / CHUNK_SIZE) - 1,
                static_cast<i32>(pos.z / CHUNK_SIZE) - 1
        };

        if (std::abs(static_cast<i32>(pos.x)) % CHUNK_SIZE > CHUNK_SIZE / 2)
            root.x += pos.x > 0 ? 1 : -1;

        if (std::abs(static_cast<i32>(pos.z)) % CHUNK_SIZE > CHUNK_SIZE / 2)
            root.y += pos.z > 0 ? 1 : -1;

        auto arr = std::array {
                this->active_chunks[INDEX(root.x,     root.y)],
                this->active_chunks[INDEX(root.x + 1, root.y)],
                this->active_chunks[INDEX(root.x,     root.y + 1)],
                this->active_chunks[INDEX(root.x + 1, root.y + 1)]
        };

        return std::make_optional(std::move(arr));
    }
}