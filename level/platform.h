//
// Created by Luis Ruisinger on 18.02.24.
//

#ifndef OPENGL_3D_ENGINE_PLATFORM_H
#define OPENGL_3D_ENGINE_PLATFORM_H

#include <map>
#include <queue>

#include "../util/aliases.h"
#include "../util/tickable.h"
#include "Chunk/chunk.h"
#include "../rendering/renderer.h"

namespace core::level {
    class Platform : util::Tickable {
    public:

        Platform(rendering::Renderer &renderer);

        // -------------------------------------
        // deallocates and write back all _loadedChunks

        ~Platform() = default;

        // ----------------------------------
        // loads and allocates initial _loadedChunks

        auto init() -> void;

        // ---------------------------------------------------------------------------------------------
        // checks if the position of the camera has reached a certain threshold for rendering new _loadedChunks
        // extracts visible faces

        auto tick() -> void override;

        // ---------------------------------
        // inserts a voxel into the platform

        auto insert(vec3f point, u16 voxelID) -> void;

        // ---------------------------------
        // removes a voxel from the platform

        auto remove(vec3f point) -> void;

        auto getBase() const -> vec2f;

        auto getRenderer() const -> const rendering::Renderer &;

    private:

        // -------------------------------
        // information about the root node

        std::array<std::unique_ptr<chunk::Chunk>, static_cast<u32>(RENDER_DISTANCE * RENDER_DISTANCE * 2 * 2)> _loadedChunks;
        vec2f                               _currentRoot;

        // ----------------------
        // handle to the _renderer

        rendering::Renderer                 &_renderer;
    };
}


#endif //OPENGL_3D_ENGINE_PLATFORM_H
