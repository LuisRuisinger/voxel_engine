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

        static auto init_fun = [&](Init) -> PlatformState {
            this->new_root = new_root_candidate;
            load_chunks(state.chunk_tick_pool);
            return Loading {};
        };

        static auto idle_fun = [&](Idle) -> PlatformState {
            if (this->queue_ready || !LOAD_THRESHOLD(this->current_root, new_root_candidate))
                return Idle {};

            if (!state.chunk_tick_pool.no_tasks())
                return Idle {};

            this->new_root = new_root_candidate;
            load_chunks(state.chunk_tick_pool);
            return Loading {};
        };

        static auto loading_fun = [&](Loading) -> PlatformState {
            if (!state.chunk_tick_pool.no_tasks())
                return Loading {};

            compress_chunks(state.chunk_tick_pool);
            return Compressing {};
        };

        static auto compressing_fun = [&](Compressing) -> PlatformState  {
            if (!state.chunk_tick_pool.no_tasks())
                return Compressing {};

            swap_chunks();
            return Swapping {};
        };

        static auto swapping_fun = [&](Swapping) -> PlatformState  {
            unload_chunks(state.chunk_tick_pool);
            return Unloading {};
        };

        static auto unloading_fun = [&](Unloading) -> PlatformState  {
            if (!state.chunk_tick_pool.no_tasks())
                return Unloading {};

            return Idle {};
        };

        static auto visitor = overload {
            init_fun, idle_fun, loading_fun, compressing_fun, swapping_fun, unloading_fun
        };

        this->platform_state = std::visit(visitor, this->platform_state);
    }

    /**
     * @brief Unload chunks whose shared count is 1, meaning they are no in use in this cycle.
     * @param thread_pool Threadpool to parallel destroy unused chunks.
     */
    auto Platform::unload_chunks(threading::thread_pool::Tasksystem<> &thread_pool) -> void {
        static auto destroy = [](chunk::Chunk *ptr) -> void {
            ASSERT_EQ(ptr);
            delete ptr;
        };

        DEBUG_LOG("Unloading chunks");

        std::vector<decltype(this->chunks)::key_type> to_erase;
        for (auto &[_, k] : this->queued_chunks) {
            if (k == nullptr)
                continue;

            bool active = false;
            for (const auto &[__, v] : this->active_chunks) {
                if (v == nullptr)
                    continue;

                if (k == v) {
                    active = true;
                    break;
                }
            }

            if (!active) {
                thread_pool.enqueue_detach(destroy, this->chunks[k].get());
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
                        std::holds_alternative<Idle>(this->platform_state)) {

                        this->queued_chunks[INDEX(x, z)] =
                                this->active_chunks[INDEX(old_pos.x, old_pos.y)];
                        init_neighbors(x, z);
                    }
                    else {

                        // uses and empty destructor because we can guarantee that the
                        // destructor lambda of the threadpool will destroy the shared pointer
                        // this needs to be done to ensure the race to 0 won't happen
                        auto ptr = std::shared_ptr<chunk::Chunk>(
                                new chunk::Chunk { static_cast<u16>(INDEX(x, z)) },
                                [](auto *){});
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
                state.render_pool.enqueue_detach(update_render_fun, k, v, state);
            }

            this->queue_ready = false;
        }
        else [[likely]] {
            for (const auto& [_, v] : this->active_chunks_vec) {
                state.render_pool.enqueue_detach(render_fun, v, state);
            }
        }

        state.render_pool.wait_for_tasks();
    }

    /** @brief Get current root position of the platform. */
    auto Platform::get_world_root() const -> glm::vec2 {
        return this->current_root;
    }

    auto Platform::get_nearest_chunks(const glm::ivec3 &pos) -> std::array<chunk::Chunk *, 4> {
        auto root = pos - glm::ivec3(this->current_root.x, 0, this->current_root.y);
        root = root / CHUNK_SIZE - 1;

        root.x += (std::abs(pos.x) % CHUNK_SIZE > CHUNK_SIZE / 2) * ((pos.x > 0) - (pos.x <= 0));
        root.z += (std::abs(pos.z) % CHUNK_SIZE > CHUNK_SIZE / 2) * ((pos.z > 0) - (pos.z <= 0));

        return {
            this->active_chunks[INDEX(root.x,     root.z)],
            this->active_chunks[INDEX(root.x + 1, root.z)],
            this->active_chunks[INDEX(root.x,     root.z + 1)],
            this->active_chunks[INDEX(root.x + 1, root.z + 1)]
        };
    }
}