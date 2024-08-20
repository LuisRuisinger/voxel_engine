//
// Created by Luis Ruisinger on 18.02.24.
//

#include <ranges>

#include "platform.h"
#include "../rendering/interface.h"
#include "../util/assert.h"

#define INDEX(_x, _z) \
    ((((_x) + RENDER_RADIUS)) + (((_z) + RENDER_RADIUS) * (2 * RENDER_RADIUS)))

#define POSITION(_i, _r) \
    (glm::vec2 { (((_i) % ((_r) * 2)) - (_r)), \
                 (((_i) / ((_r) * 2)) - (_r))})

#define DISTANCE_2D(_p1, _p2) \
    (std::hypot((_p1).x - (_p2).x, (_p1).y - (_p2).y))

#define LOAD_THRESHOLD(_p1, _p2) \
    (DISTANCE_2D((_p1), (_p2)) == CHUNK_SIZE * 2)

namespace core::level {
    Platform::Platform(presenter::Presenter &_presenter)
        : presenter { _presenter }
    {}

    /**
     * @brief Update platform if needed. Load new chunks if threshold is hit.
     * @param thread_pool Pool to offload tasks.
     * @param camera Current active camera.
     */
    auto Platform::tick(
            threading::Tasksystem<> &thread_pool __attribute__((noescape)),
            camera::perspective::Camera &camera __attribute__((noescape)))
            -> void {
        const auto &cameraPos = camera.getCameraPosition();
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
    auto Platform::unload_chunks(
            threading::Tasksystem<> &thread_pool __attribute__((noescape))) -> Platform & {
        static auto destroy = [](std::shared_ptr<chunk::Chunk> ptr) -> void {
            ASSERT_EQ(ptr.get());
            static_cast<void>(ptr);
        };

        DEBUG_LOG("Unloading chunks");
        for (size_t i = 0; i < this->queued_chunks.size(); ++i) {
            if (this->queued_chunks[i] &&
                this->queued_chunks[i].use_count() == 2)
                thread_pool.enqueue_detach(
                        destroy,
                        std::move(this->queued_chunks[i]));
        }

        DEBUG_LOG("Finished unloading chunks");
        return *this;
    }

    /**
     * @brief Load new chunks and share ownership of currently loaded chunks that are still
     *        visible in the new region.
     * @param thread_pool Threadpool to parallel generate new chunks.
     * @param new_root    The center of the new region.
     */
    auto Platform::load_chunks(
            threading::Tasksystem<> &thread_pool __attribute__((noescape)),
            glm::vec2 new_root)
            -> Platform & {
        static auto generate = [](
                chunk::Chunk *ptr __attribute__((noescape)),
                Platform *platform) -> void {
            ASSERT_EQ(ptr);
            ptr->generate(platform);
        };

        for (i32 x = -RENDER_RADIUS; x < RENDER_RADIUS; ++x) {
            for (i32 z = -RENDER_RADIUS; z < RENDER_RADIUS; ++z) {
                if (DISTANCE_2D(glm::vec2(-0.5), glm::vec2(x, z)) < RENDER_RADIUS) {
                    auto old_pos =
                            (glm::vec2(x, z) * static_cast<f32>(CHUNK_SIZE) +
                            new_root -
                            current_root) / static_cast<f32>(CHUNK_SIZE);

                    if (DISTANCE_2D(glm::vec2(-0.5), old_pos) < RENDER_RADIUS &&
                        this->platform_ready) [[likely]] {

                        std::unique_lock lock { this->mutex };
                        ASSERT_EQ(this->active_chunks[INDEX(old_pos.x, old_pos.y)].get());
                        this->queued_chunks[INDEX(x, z)] =
                                this->active_chunks[INDEX(old_pos.x, old_pos.y)];

                        add_neighbour(old_pos, x, z);
                    }
                    else {
                        this->queued_chunks[INDEX(x, z)] =
                                std::make_shared<chunk::Chunk>(INDEX(x, z));

                        add_neighbour(old_pos, x, z);
                        thread_pool.enqueue_detach(
                                generate,
                                this->queued_chunks[INDEX(x, z)].get(),
                                this);
                    }

                }
            }
        }

        thread_pool.wait_for_tasks(std::chrono::milliseconds(0));
        DEBUG_LOG("Finished chunk initialization");
        return *this;
    }

    auto Platform::add_neighbour(glm::vec2 root, i32 x, i32 z) -> void {
        if (INDEX(x - 1, z) > -1 &&
            this->queued_chunks[INDEX(x - 1, z)]) {
            this->queued_chunks[INDEX(x, z)]->add_neigbor(
                    chunk::Position::BACK,
                    this->queued_chunks[INDEX(x - 1, z)]);

            this->queued_chunks[INDEX(x - 1, z)]->add_neigbor(
                    chunk::Position::FRONT,
                    this->queued_chunks[INDEX(x, z)]);
        }

        if (INDEX(x + 1, z) < MAX_RENDER_VOLUME &&
            this->queued_chunks[INDEX(x + 1, z)]) {
            this->queued_chunks[INDEX(x, z)]->add_neigbor(
                    chunk::Position::FRONT,
                    this->queued_chunks[INDEX(x + 1, z)]);

            this->queued_chunks[INDEX(x + 1, z)]->add_neigbor(
                    chunk::Position::BACK,
                    this->queued_chunks[INDEX(x, z)]);
        }

        if (INDEX(x, z - 1) > -1 &&
            this->queued_chunks[INDEX(x, z - 1)]) {
            this->queued_chunks[INDEX(x, z)]->add_neigbor(
                    chunk::Position::LEFT,
                    this->queued_chunks[INDEX(x, z - 1)]);

            this->queued_chunks[INDEX(x, z - 1)]->add_neigbor(
                    chunk::Position::RIGHT,
                    this->queued_chunks[INDEX(x, z)]);
        }

        if (INDEX(x, z + 1) < MAX_RENDER_VOLUME &&
            this->queued_chunks[INDEX(x, z + 1)]) {
            this->queued_chunks[INDEX(x, z)]->add_neigbor(
                    chunk::Position::RIGHT,
                    this->queued_chunks[INDEX(x, z + 1)]);

            this->queued_chunks[INDEX(x, z + 1)]->add_neigbor(
                    chunk::Position::LEFT,
                    this->queued_chunks[INDEX(x, z)]);
        }
    }

    /**
    * @brief Sliding window principle to swap active chunks with the new region.
    * @param new_root The center of the new region.
    */
    auto Platform::swap_chunks(glm::vec2 new_root) -> Platform & {
        std::unique_lock lock{ this->mutex };
        std::swap(this->active_chunks, this->queued_chunks);

        this->current_root = new_root;
        this->queue_ready = true;
        this->platform_ready = true;

        DEBUG_LOG("Chunks swapped");
        return *this;
    }

    /**
     * @brief Extract visible mesh for current frame.
     * @param thread_pool Parallel traversal of single chunks.
     * @param camera      Active camera for this frame.
     */
    auto Platform::frame(
            threading::Tasksystem<> &thread_pool,
            camera::perspective::Camera &camera)
            -> void {
        static auto update_render_fun = [](
                u16 idx,
                chunk::Chunk &ptr,
                camera::perspective::Camera &camera,
                Platform &platform) -> void {
            ptr.update_and_render(idx, camera, platform);
        };

        static auto render_fun = [](
                chunk::Chunk &ptr,
                camera::perspective::Camera &camera,
                Platform &platform) -> void {
            ptr.cull(camera, platform);
        };

        // check if new chunks need to be added to the active pool
        // try lock to ensure no amount of frame freeze happens
        std::unique_lock lock { this->mutex };

        if (this->queue_ready) {
            for (size_t i = 0; i < active_chunks.size(); ++i)
                if (active_chunks[i])
                    thread_pool.enqueue_detach(
                            update_render_fun,
                            i,
                            std::ref(*this->active_chunks[i]),
                            std::ref(camera),
                            std::ref(*this));

            this->queue_ready = false;
        }
        else [[likely]] {
            for (size_t i = 0; i < active_chunks.size(); ++i)
                if (active_chunks[i] && active_chunks[i]->visible(camera, *this))
                    thread_pool.enqueue_detach(
                            render_fun,
                            std::ref(*this->active_chunks[i].get()),
                            std::ref(camera),
                            std::ref(*this));
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

    /**
     * @brief  Codegen proxy for underlying chunks.
     *         Applies function on all chunks if no position is contained in the args.
     * @tparam Func Function pointer type.
     * @tparam Args Argument types.
     * @param  func Function pointer to a member of chunk.
     * @param  args Arguments to resolve the right function.
     * @return Equal to the called function through the proxy.
     */
    template <typename Func, typename ...Args>
    requires util::reflections::has_member_v<chunk::Chunk, Func>
    INLINE auto Platform::request_handle(
            threading::Tasksystem<> &thread_pool __attribute__((noescape)),
            Func func,
            Args &&...args) const
            -> std::invoke_result_t<decltype(func), chunk::Chunk*, Args...> {
        using namespace util::reflections;
        constexpr auto args_tuple = tuple_from_params(std::forward<Args>(args)...);

        if constexpr (has_type_v<glm::vec3, decltype(args_tuple)>) {
            auto &[x, _, z] = std::get<glm::vec3>(args_tuple);

            // preprocessing
            auto idx = INDEX(
                    (static_cast<i32>(x / CHUNK_SIZE) - (this->current_root.x / CHUNK_SIZE)),
                    (static_cast<i32>(z / CHUNK_SIZE) - (this->current_root.y / CHUNK_SIZE)));

            x = static_cast<i32>(x) % CHUNK_SIZE;
            z = static_cast<i32>(z) % CHUNK_SIZE;

            return (active_chunks[idx].get()->*func)(std::forward<Args>(args)...);
        }
        else {
            for (auto &chunk : this->active_chunks)
                thread_pool.enqueue_detach(
                        std::move([](
                                std::weak_ptr<chunk::Chunk> chunk,
                                Func func,
                                Args ...args) -> void {
                            if (auto s_ptr = chunk.lock())
                                (s_ptr.get()->*func)(std::forward<Args>(args)...);
                            }),
                        chunk,
                        std::forward<Func>(func),
                        std::forward<Args>(args)...);
        }
    }
}