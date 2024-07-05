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

namespace core::level {

    Platform::Platform(presenter::Presenter &_presenter)
        : presenter{_presenter}
    {}

    /**
     * @brief Update platform if needed. Load new chunks if threshold is hit.
     *
     * @param thread_pool Pool to offload tasks.
     * @param camera Current active camera.
     */

    auto Platform::tick(
            threading::Tasksystem<> &thread_pool __attribute__((noescape)),
            camera::perspective::Camera &camera __attribute__((noescape)))
            -> void {
        const auto &cameraPos = camera.getCameraPosition();

        const auto new_root = glm::vec2{
            static_cast<i32>(std::lround(static_cast<i32>(cameraPos.x / 32.0F)) * 32.0F),
            static_cast<i32>(std::lround(static_cast<i32>(cameraPos.z / 32.0F)) * 32.0F)
        };

        ASSERT(static_cast<i32>(new_root.x) % 32 == 0, "global platform roots must be multiple of 32");
        ASSERT(static_cast<i32>(new_root.y) % 32 == 0, "global platform roots must be multiple of 32");

        // threshold to render new chunks is double the chunk size
        if ((!this->queue_ready && DISTANCE_2D(this->current_root, new_root) == 64.0F) ||
             !this->platform_ready) {

            util::log::out() << util::log::Level::LOG_LEVEL_NORMAL
                             << new_root
                             << util::log::end;

            load_chunks(thread_pool, new_root);
            swap_chunks(new_root);
            unload_chunks(thread_pool);
        }
    }

    /**
     * @brief Unload chunks whose shared count is 1, meaning they are no in use in this cycle.
     *
     * Through abusing the characteristics of shared_ptr we can enusre that only valid unused chunks
     * will be destroyed. Chunks that are still in use but are also contained in this queue won't be
     * altered due to the reference count being greater than 1.
     *
     * @param thread_pool Threadpool to parallel destroy unused chunks.
     */

    auto Platform::unload_chunks(threading::Tasksystem<> &thread_pool __attribute__((noescape))) -> void {
        for (size_t i = 0; i < this->queued_chunks.size(); ++i) {
            if (this->queued_chunks[i] && this->queued_chunks[i].use_count() == 2)
                thread_pool.enqueue_detach(
                        std::move([](std::shared_ptr<chunk::Chunk> ptr) -> void {
                            static_cast<void>(ptr);
                        }),

                        std::move(this->queued_chunks[i]));
        }
    }

    /**
     * @brief Load new chunks and share ownership of currently loaded chunks that are still
     *        visible in the new region.
     *
     * Chunks are guaranteed to be entirely generated once being enqueued in the thread pool.
     *
     * @param thread_pool Threadpool to parallel generate new chunks.
     * @param new_root    The center of the new region.
     */

    auto Platform::load_chunks(
            threading::Tasksystem<> &thread_pool __attribute__((noescape)),
            glm::vec2 new_root)
            -> void {
        for (i32 x = -RENDER_RADIUS; x < RENDER_RADIUS; ++x) {
            for (i32 z = -RENDER_RADIUS; z < RENDER_RADIUS; ++z) {
                if (DISTANCE_2D(glm::vec2(-0.5), glm::vec2(x, z)) < RENDER_RADIUS) {
                    auto old_pos = (((glm::vec2(x, z) * 32.0F) + new_root) - current_root) / 32.0F;

                    if (DISTANCE_2D(glm::vec2(-0.5), old_pos) < RENDER_RADIUS &&
                        this->platform_ready) [[likely]] {

                        std::unique_lock lock {this->mutex};
                        ASSERT(this->active_chunks[INDEX(old_pos.x, old_pos.y)].get(), "");
                        this->queued_chunks[INDEX(x, z)] = this->active_chunks[INDEX(old_pos.x, old_pos.y)];
                    }
                    else {
                        this->queued_chunks[INDEX(x, z)] = std::make_shared<chunk::Chunk>(INDEX(x, z));
                        thread_pool.enqueue_detach(
                                std::move([](
                                        chunk::Chunk *ptr __attribute__((noescape)),
                                        Platform *platform) -> void {
                                    ASSERT(ptr);
                                    ptr->generate(platform);
                                }),

                                this->queued_chunks[INDEX(x, z)].get(),
                                this);
                    }

                }
            }
        }

        for (auto &ref : this->queued_chunks)
            ASSERT(ref.use_count() < 3, "invalid reference count " + std::to_string(ref.use_count()));

        thread_pool.wait_for_tasks(std::chrono::milliseconds(0));
    }

    /**
    * @brief Sliding window principle to swap active chunks with the new region.
    *
    * @param new_root    The center of the new region.
    */

    auto Platform::swap_chunks(glm::vec2 new_root) -> void {
        std::unique_lock lock{this->mutex};
        std::swap(this->active_chunks, this->queued_chunks);

        this->current_root   = new_root;
        this->queue_ready    = true;
        this->platform_ready = true;

        LOG("Chunks swapped");
    }

    /**
     * @brief Extract visible mesh for current frame.
     *
     * @param thread_pool Parallel traversal of single chunks.
     * @param camera      Active camera for this frame.
     */

    auto Platform::frame(
            threading::Tasksystem<> &thread_pool __attribute__((noescape)),
            camera::perspective::Camera &camera)
            -> void {

        // check if new chunks need to be added to the active pool
        // try lock to ensure no amount of frame freeze happens
        std::unique_lock lock {this->mutex};

        if (this->queue_ready) {
            for (size_t i = 0; i < active_chunks.size(); ++i)

                // we need to update every chunk here, thus we don't filter for visible chunks
                if (active_chunks[i]) {
                    thread_pool.enqueue_detach(
                            std::move([](
                                    u16 idx,
                                    chunk::Chunk &ptr __attribute__((noescape)),
                                    camera::perspective::Camera &camera,
                                    Platform &platform) -> void {
                                ptr.update_and_render(idx, camera, platform);
                            }),

                            static_cast<u16>(i),
                            std::ref(*this->active_chunks[i].get()),
                            std::ref(const_cast<camera::perspective::Camera &>(camera)),
                            std::ref(*this)
                    );
                }

            this->queue_ready = false;
        }
        else [[likely]] {

            // else we just render normally
            for (size_t i = 0; i < active_chunks.size(); ++i)
                if (active_chunks[i] && active_chunks[i]->visible(camera, *this)) {
                    thread_pool.enqueue_detach(
                            std::move([](
                                    chunk::Chunk &ptr __attribute__((noescape)),
                                    camera::perspective::Camera &camera,
                                    Platform &platform) -> void {
                                ptr.cull(camera, platform);
                            }),

                            std::ref(*this->active_chunks[i].get()),
                            std::ref(const_cast<camera::perspective::Camera &>(camera)),
                            std::ref(*this)


                    );
                }
        }

        thread_pool.wait_for_tasks(std::chrono::milliseconds(0));
        ASSERT(thread_pool.no_tasks());
    }

    auto Platform::insert(glm::vec3 point, u16 voxelID) -> void {
        //this->loadedChunks->insert(point, _voxelID);
    }

    auto Platform::remove(glm::vec3 point) -> void {
        //this->loadedChunks->remove(point);
    }

    auto Platform::getBase() const -> glm::vec2 {
        return this->current_root;
    }

    auto Platform::get_presenter() const -> presenter::Presenter & {
        return this->presenter;
    }
}