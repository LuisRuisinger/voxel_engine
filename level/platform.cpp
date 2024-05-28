//
// Created by Luis Ruisinger on 18.02.24.
//

#include "platform.h"

#define INDEX(_x, _y) ((((_x) + RENDER_RADIUS)) + (((_y) + RENDER_RADIUS) * (2 * RENDER_RADIUS)))

namespace core::level {

    //
    //
    //

    // ----------------
    // helper functions

    static inline
    auto calculateDistance2D(const glm::vec2 &p1, const glm::vec2 &p2) -> f32 {
        return std::hypot(p1.x - p2.x, p1.y - p2.y);
    }

    //
    //
    //

    // -----------------------
    // platform implementation

    Platform::Platform(rendering::Renderer &renderer)
        : _renderer{renderer}
        , _currentRoot{renderer.getCamera()->getCameraPosition().x, renderer.getCamera()->getCameraPosition().z}
        , _loadedChunks{}
    {}

    auto Platform::init() -> void {
        for (i32 x = -RENDER_RADIUS; x < RENDER_RADIUS; ++x)
            for (i32 z = -RENDER_RADIUS; z < RENDER_RADIUS; ++z)
                if (calculateDistance2D(glm::vec2 {-0.5}, {x, z}) < RENDER_RADIUS)
                    _loadedChunks[INDEX(x, z)] = std::make_unique<chunk::Chunk>(INDEX(x, z), this);

        u16 idx = 0;
        for (auto &x : _loadedChunks) {
            if (x)
                x->update(idx);
            ++idx;
        }
    }

    auto Platform::tick(threading::Tasksystem<> &thread_pool) -> void {
        const auto &camera    = *_renderer.getCamera();
        const auto &cameraPos = camera.getCameraPosition();

        i32 nearestChunkX = static_cast<i32>(static_cast<i32>(cameraPos.x) / CHUNK_SIZE) * CHUNK_SIZE;
        i32 nearestChunkZ = static_cast<i32>(static_cast<i32>(cameraPos.z) / CHUNK_SIZE) * CHUNK_SIZE;

        const auto newRoot = glm::vec2 {
                (abs(nearestChunkX - static_cast<i32>(_currentRoot.x)) > CHUNK_SIZE) ? nearestChunkX : _currentRoot.x,
                (abs(nearestChunkZ - static_cast<i32>(_currentRoot.y)) > CHUNK_SIZE) ? nearestChunkZ : _currentRoot.y
        };

        // ----------------------------------------------------------------------------------------------------
        // checks if the position of the camera has reached a certain threshold for rendering new _loadedChunks

        if (calculateDistance2D(_currentRoot, newRoot) > CHUNK_SIZE) {

            // -------------------------------------------------------
            // calculates all existing _loadedChunks inside our render radius

            for (i32 x = -RENDER_RADIUS; x < RENDER_RADIUS; ++x) {
                for (i32 z = -RENDER_RADIUS; z < RENDER_RADIUS; ++z) {

                    // ---------------------------------------------------------------------------------------
                    // if the old camera position contains _loadedChunks we need in the new state we can extract them

                    // TODO: steal old chunks

                    if (calculateDistance2D(glm::vec2 {-0.5}, {x, z}) < RENDER_RADIUS)
                        _loadedChunks[INDEX(x, z)] = std::make_unique<chunk::Chunk>(INDEX(x, z), this);
                }
            }

            _currentRoot = newRoot;

            u16 idx = 0;
            for (auto &x : _loadedChunks) {
                if (x)
                    x->update(idx);
                ++idx;
            }
        }

        auto filtered = _loadedChunks |
                        std::views::filter([](const auto &ptr) -> bool { return ptr.operator bool(); }) |
                        std::views::filter([this, &camera](const auto &ptr) -> bool { return ptr->visible(camera, *this); });

        /*
         * render steps :
         *
         * 1. determine visible chunks via (x,z) culling
         * 2. determine visible chunsk via occlusion query from last frame
         * 3. allocate buffer of size (sqrt(#visible chunks) * 32)^2 * 3 * 4 (3 : 3 planes visible, 4 : amount of vertices per face)
         */

        std::ranges::for_each(
                filtered,
                [&thread_pool, &camera, this](const auto &ptr) mutable -> void {
                    thread_pool.enqueue_detach(

                            // extracting voxels for rendering
                            [](chunk::Chunk &ptr, camera::perspective::Camera &camera, Platform &platform) -> void {
                                ptr.cull(camera, platform);
                            },

                            // args
                            std::ref(*ptr.get()),
                            std::ref(const_cast<camera::perspective::Camera &>(camera)),
                            std::ref(*this));
                });

        thread_pool.wait_for_tasks(std::chrono::milliseconds(8));
    }

    auto Platform::insert(glm::vec3 point, u16 voxelID) -> void {
        //this->loadedChunks->insert(point, _voxelID);
    }

    auto Platform::remove(glm::vec3 point) -> void {
        //this->loadedChunks->remove(point);
    }

    auto Platform::getBase() const -> glm::vec2 {
        return _currentRoot;
    }

    auto Platform::getRenderer() const -> const rendering::Renderer & {
        return _renderer;
    }
}