//
// Created by Luis Ruisinger on 18.02.24.
//

#ifndef OPENGL_3D_ENGINE_PLATFORM_H
#define OPENGL_3D_ENGINE_PLATFORM_H

#include <map>
#include <queue>

#include "chunk/chunk.h"

#include "../rendering/renderer.h"
#include "../threading/thread_pool.h"

#include "../../util/aliases.h"
#include "../../util/reflections.h"

#define MAX_RENDER_VOLUME (static_cast<u32>(RENDER_RADIUS * RENDER_RADIUS * 2 * 2))

namespace core::level::presenter {
    class Presenter;
}

namespace core::level::platform {

    class Platform {
    public:
        Platform(presenter::Presenter &presenter);
        ~Platform() =default;

        auto tick(
                threading::thread_pool::Tasksystem<> &,
                util::camera::Camera &) -> void;
        auto frame(
                threading::thread_pool::Tasksystem<> &,
                util::camera::Camera &) -> void;
        auto get_world_root() const -> glm::vec2;
        auto get_presenter() const -> presenter::Presenter &;

        template <
                typename Func,
                typename ...Args,
                typename Ret = std::invoke_result_t<Func, chunk::Chunk*, Args...>>
        requires util::reflections::has_member_v<chunk::Chunk, Func>
        INLINE auto request_handle(threading::thread_pool::Tasksystem<> &,
                                   Func func,
                                   Args &&...args) const -> Ret {
            using namespace util::reflections;
            constexpr auto args_tuple = tuple_from_params(std::forward<Args>(args)...);
        }

        auto get_visible_faces(util::camera::Camera &camera) -> size_t;

    private:
        auto unload_chunks(threading::thread_pool::Tasksystem<> &) -> Platform &;
        auto load_chunks(threading::thread_pool::Tasksystem<> &, glm::vec2) -> Platform &;
        auto swap_chunks(glm::vec2) -> Platform &;
        auto init_chunk_neighbours(i32,
                                   i32,
                                   chunk::Position,
                                   chunk::Position) -> void;

        std::unordered_map<chunk::Chunk *, std::shared_ptr<chunk::Chunk>> chunks;
        std::unordered_map<u32, chunk::Chunk *> active_chunks;
        std::unordered_map<u32, chunk::Chunk *> queued_chunks;

        std::vector<std::pair<u32, chunk::Chunk *>> active_chunks_vec;

        glm::vec2                                   current_root   = {0.0F, 0.0F};
        presenter::Presenter                       &presenter;
        std::mutex                                  mutex;
        std::atomic_bool                            queue_ready    = false;
        std::atomic_bool                            platform_ready = false;
    };
}


#endif //OPENGL_3D_ENGINE_PLATFORM_H