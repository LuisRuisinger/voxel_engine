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
#include "../util/reflections.h"

#define MAX_RENDER_VOLUME (static_cast<u32>(RENDER_RADIUS * RENDER_RADIUS * 2 * 2))


namespace core::level {
    namespace presenter {
        class Presenter;
    }

    class Platform : public util::observer::Observer {
    public:
        Platform(presenter::Presenter &presenter);
        ~Platform() override =default;

        auto tick(
                threading::Tasksystem<> & __attribute__((noescape)),
                camera::perspective::Camera & __attribute__((noescape))) -> void override;
        auto frame(
                threading::Tasksystem<> &,
                camera::perspective::Camera &) -> void;
        auto get_world_root() const -> glm::vec2;
        auto get_presenter() const -> presenter::Presenter &;

        template <typename Func, typename ...Args>
        requires util::reflections::has_member_v<chunk::Chunk, Func>
        INLINE auto request_handle(
                threading::Tasksystem<> & __attribute__((noescape)),
                Func func,
                Args &&...args) const
        -> std::invoke_result_t<decltype(func), chunk::Chunk*, Args...>;

        auto get_visible_faces(camera::perspective::Camera &camera) -> size_t;

    private:
        auto unload_chunks(threading::Tasksystem<> & __attribute__((noescape))) -> Platform &;
        auto load_chunks(threading::Tasksystem<> & __attribute__((noescape)), glm::vec2) -> Platform &;
        auto swap_chunks(glm::vec2) -> Platform &;
        auto add_neighbour(glm::vec2, i32, i32) -> void;

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
