//
// Created by Luis Ruisinger on 18.02.24.
//

#include "Platform.h"

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

    static inline
    auto renderBoundingVolume(vec2f position) -> std::tuple<i32, i32, i32, i32> {
        return {
                -static_cast<f32>(RENDER_RADIUS * CHUNK_SIZE) + position.x,
                 static_cast<f32>(RENDER_RADIUS * CHUNK_SIZE) - CHUNK_SIZE + position.x,
                -static_cast<f32>(RENDER_RADIUS * CHUNK_SIZE) + position.y,
                 static_cast<f32>(RENDER_RADIUS * CHUNK_SIZE) - CHUNK_SIZE + position.y
        };
    }

    //
    //
    //

    // -----------------------
    // platform implementation

    Platform::Platform(Renderer::Renderer &other)
        : renderer{other}
        , currentRoot{other.getCamera()->getCameraPosition().x,
                      other.getCamera()->getCameraPosition().z}
        , loadedChunks{std::make_unique<Quadtree::Handler>(this->currentRoot)} {}

    auto Platform::init() -> void {
        const auto [xMin, xMax, zMin, zMax] = renderBoundingVolume(this->currentRoot);

        // -------------------------------------------------------
        // calculates all existing chunks inside our render radius

        for (i32 x = xMin; x <= xMax; x += CHUNK_SIZE) {
            for (i32 z = zMin; z <= zMax; z += CHUNK_SIZE) {
                if (calculateDistance2D(this->currentRoot, {x, z}) < RENDER_DISTANCE) {
                    this->loadedChunks->insertChunk({x, z});
                }
            }
        }

        this->loadedChunks->update();
    }

    auto Platform::tick(Camera::Camera& camera) -> void {

        // ----------------------------------------------------------
        // rounds the camera's position to the nearest chunk position

        const auto nCurrentRoot  = glm::vec2 {
            (static_cast<i32>(camera.getCameraPosition().x)) % CHUNK_SIZE > CHUNK_SIZE / 2
            ? (static_cast<i32>(camera.getCameraPosition().x) / CHUNK_SIZE) * CHUNK_SIZE + CHUNK_SIZE
            : (static_cast<i32>(camera.getCameraPosition().x) / CHUNK_SIZE) * CHUNK_SIZE,
            (static_cast<i32>(camera.getCameraPosition().z)) % CHUNK_SIZE > CHUNK_SIZE / 2
            ? (static_cast<i32>(camera.getCameraPosition().z) / CHUNK_SIZE) * CHUNK_SIZE + CHUNK_SIZE
            : (static_cast<i32>(camera.getCameraPosition().z) / CHUNK_SIZE) * CHUNK_SIZE
        };

        // ---------------------------------------------------------------------------------------------
        // checks if the position of the camera has reached a certain threshold for rendering new chunks


        if (calculateDistance2D(this->currentRoot, nCurrentRoot) > CHUNK_SIZE) {
            const auto [xMin, xMax, zMin, zMax] = renderBoundingVolume(nCurrentRoot);
            auto nLoadedChunks = std::make_unique<Quadtree::Handler>(nCurrentRoot);

            // -------------------------------------------------------
            // calculates all existing chunks inside our render radius

            for (i32 x = xMin; x <= xMax; x += CHUNK_SIZE) {
                for (i32 z = zMin; z <= zMax; z += CHUNK_SIZE) {

                    // ---------------------------------------------------------------------------------------
                    // if the old camera position contains chunks we need in the new state we can extract them

                    if (calculateDistance2D(nCurrentRoot, {x, z}) < RENDER_DISTANCE) {
                        if (calculateDistance2D(this->currentRoot, {x, z}) < RENDER_DISTANCE) {
                            auto chunk = loadedChunks->extractChunk({x, z});
                            nLoadedChunks->insertExtractedChunk(chunk);
                        }
                        else {
                            nLoadedChunks->insertChunk({x, z});
                        }
                    }
                }
            }

            nLoadedChunks->update();

            this->loadedChunks = std::move(nLoadedChunks);
            this->currentRoot  = nCurrentRoot;
        }

        // -----------------------------------
        // extract visible faces for this tick

        this->loadedChunks->cull(camera, this->renderer);
    }

    auto Platform::insert(vec3f point, u16 voxelID) -> void {
        this->loadedChunks->insert(point, voxelID);
    }

    auto Platform::remove(vec3f point) -> void {
        this->loadedChunks->remove(point);
    }
}