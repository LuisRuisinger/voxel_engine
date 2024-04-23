//
// Created by Luis Ruisinger on 18.02.24.
//

#include "Platform.h"

#define INDEX(_p) (static_cast<u32>((_p).x + (_p).y * CHUNK_SIZE))

namespace Platform {

    //
    //
    //

    // ----------------
    // helper functions

    static inline
    auto calculateDistance2D(const vec2f &p1, const vec2f &p2) -> f32 {
        return static_cast<f32>(sqrt(pow((p1.x - p2.x), 2) + pow((p1.y - p2.y), 2)));
    }

    //
    //
    //

    // -----------------------
    // platform implementation

    Platform::Platform(Renderer::Renderer &renderer)
        : _renderer{renderer}
        , _currentRoot{renderer.getCamera()->getCameraPosition().x, renderer.getCamera()->getCameraPosition().z}
        , _loadedChunks{}
    {}

    auto Platform::init() -> void {
        for (i32 x = -RENDER_RADIUS; x < RENDER_RADIUS; ++x) {
            for (i32 z = -RENDER_RADIUS; z < RENDER_RADIUS; ++z) {
                if (calculateDistance2D(vec2f {0}, {x, z}) < RENDER_RADIUS) {
                    auto pos = vec2f {x, z};
                    auto deffered = pos + vec2f {RENDER_RADIUS};

                    _loadedChunks[INDEX(deffered)] = std::make_unique<Chunk::Chunk>(pos, this);
                }
            }
        }

        std::ranges::for_each(
                _loadedChunks | std::views::filter([](const auto &ptr) -> bool { return ptr.operator bool(); }),
                [](const auto &ptr) -> void { ptr->update(); });
    }

    auto Platform::tick(Camera::Camera &camera) -> void {
        auto filtered = _loadedChunks
                        | std::views::filter([](const auto &ptr) -> bool { return ptr.operator bool(); });

        // ----------------------------------------------------------
        // rounds the camera's position to the nearest chunk position

        const auto &cameraPos = camera.getCameraPosition();

        i32 nearestChunkX = static_cast<i32>(static_cast<i32>(cameraPos.x) / CHUNK_SIZE) * CHUNK_SIZE;
        i32 nearestChunkZ = static_cast<i32>(static_cast<i32>(cameraPos.z) / CHUNK_SIZE) * CHUNK_SIZE;

        const auto newRoot = glm::vec2 {
                (abs(nearestChunkX - static_cast<i32>(_currentRoot.x)) > CHUNK_SIZE)
                ? nearestChunkX : _currentRoot.x,
                (abs(nearestChunkZ - static_cast<i32>(_currentRoot.y)) > CHUNK_SIZE)
                ? nearestChunkZ : _currentRoot.y
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

                    // TODO

                    if (calculateDistance2D(vec2f {0}, {x, z}) < RENDER_RADIUS) {
                        auto pos = vec2f {x, z};
                        auto deffered = pos + vec2f {RENDER_RADIUS};

                        _loadedChunks[INDEX(deffered)] = std::make_unique<Chunk::Chunk>(pos, this);
                    }
                }
            }

            _currentRoot = newRoot;

            std::ranges::for_each(
                    filtered,
                    [](const auto &ptr) -> void { ptr->update(); });
        }

        _renderer.updateGlobalBase(_currentRoot);

        std::ranges::for_each(
                filtered,
                [&camera, this](const auto &ptr) -> void { (void) ptr->cull(camera, *this); });
    }

    auto Platform::insert(vec3f point, u16 voxelID) -> void {
        //this->loadedChunks->insert(point, _voxelID);
    }

    auto Platform::remove(vec3f point) -> void {
        //this->loadedChunks->remove(point);
    }

    auto Platform::getBase() const -> vec2f {
        return _currentRoot;
    }

    auto Platform::getRenderer() const -> const Renderer::Renderer & {
        return _renderer;
    }
}