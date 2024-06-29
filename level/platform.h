//
// Created by Luis Ruisinger on 18.02.24.
//

#ifndef OPENGL_3D_ENGINE_PLATFORM_H
#define OPENGL_3D_ENGINE_PLATFORM_H

#include <map>
#include <queue>

#include "../util/aliases.h"
#include "../util/observer.h"
#include "chunk/chunk.h"
#include "../rendering/renderer.h"

#define MAX_RENDER_VOLUME (static_cast<u32>(RENDER_RADIUS * RENDER_RADIUS * 2 * 2))


namespace core::level {
    namespace presenter {
        class Presenter;
    }

    class Platform : public util::observer::Observer {
    public:
        Platform(presenter::Presenter &presenter);
        ~Platform() override = default;

        auto tick(threading::Tasksystem<> &, camera::perspective::Camera &) -> void override;
        auto insert(glm::vec3 point, u16 voxelID) -> void;
        auto frame(threading::Tasksystem<> &, camera::perspective::Camera &) -> void;
        auto remove(glm::vec3 point) -> void;
        auto getBase() const -> glm::vec2;
        auto get_presenter() const -> presenter::Presenter &;

    private:
        auto unload_chunks(threading::Tasksystem<> &) -> void;
        auto load_chunks(threading::Tasksystem<> &, glm::vec2) -> void;
        auto swap_chunks(glm::vec2) -> void;

        std::vector<std::shared_ptr<chunk::Chunk>> active_chunks =
                std::vector<std::shared_ptr<chunk::Chunk>>(MAX_RENDER_VOLUME);

        std::vector<std::shared_ptr<chunk::Chunk>> queued_chunks =
                std::vector<std::shared_ptr<chunk::Chunk>>(MAX_RENDER_VOLUME);

        glm::vec2                                   current_root   = {0.0F, 0.0F};
        presenter::Presenter                       &presenter;
        std::mutex                                  mutex;
        std::atomic_bool                            queue_ready    = false;
        std::atomic_bool                            platform_ready = false;
    };
}


#endif //OPENGL_3D_ENGINE_PLATFORM_H
