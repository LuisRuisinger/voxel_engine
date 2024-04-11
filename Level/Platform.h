//
// Created by Luis Ruisinger on 18.02.24.
//

#ifndef OPENGL_3D_ENGINE_PLATFORM_H
#define OPENGL_3D_ENGINE_PLATFORM_H

#include <map>
#include <queue>

#include "../global.h"
#include "Quadtree/Quadtree.h"
#include "../Rendering/Renderer.h"

namespace Platform {
    class Platform {
    public:

        Platform(Renderer::Renderer &other);

        // -------------------------------------
        // deallocates and write back all chunks

        ~Platform() = default;

        // ----------------------------------
        // loads and allocates initial chunks

        auto init() -> void;

        // ---------------------------------------------------------------------------------------------
        // checks if the position of the camera has reached a certain threshold for rendering new chunks
        // extracts visible faces

        auto tick(Camera::Camera& camera) -> void;

        // ---------------------------------
        // inserts a voxel into the platform

        auto insert(vec3f point, u16 voxelID) -> void;

        // ---------------------------------
        // removes a voxel from the platform

        auto remove(vec3f point) -> void;

    private:

        // -------------------------------
        // information about the root node

        std::unique_ptr<Quadtree::Handler>  loadedChunks;
        vec2f                               currentRoot;

        // ----------------------
        // handle to the renderer

        Renderer::Renderer                 &renderer;
    };
}


#endif //OPENGL_3D_ENGINE_PLATFORM_H
