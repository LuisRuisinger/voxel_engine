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

#include "../../util/defines.h"
#include "../../util/reflections.h"
#include "../state.h"
#include "../../util/traits.h"

#define MAX_RENDER_VOLUME (static_cast<u32>(RENDER_RADIUS * RENDER_RADIUS * 2 * 2))

namespace core::level::platform {
    using namespace util;

    struct Init {};
    struct Idle {};
    struct Loading{};
    struct Compressing{};
    struct Swapping{};
    struct Unloading{};

    template<typename ...Ts>
    struct overload : Ts... { using Ts::operator()...; };

    template<typename ...Ts>
    overload(Ts...) -> overload<Ts...>;

    using PlatformState = std::variant<Init, Idle, Loading, Compressing, Swapping, Unloading>;

    class Platform :
        public traits::Tickable<Platform>,
        public traits::Updateable<Platform> {
    public:
        Platform() =default;
        ~Platform() =default;

        auto tick(state::State &) -> void;
        auto update(state::State &state) -> void;
        auto get_world_root() const -> glm::vec2;
        auto get_visible_faces(util::camera::Camera &camera) -> size_t;
        auto get_nearest_chunks(const glm::ivec3 &) -> std::array<chunk::Chunk *, 4>;

    private:
        auto unload_chunks(threading::thread_pool::Tasksystem<> &) -> void;
        auto load_chunks(threading::thread_pool::Tasksystem<> &) -> void;
        auto compress_chunks(threading::thread_pool::Tasksystem<> &) -> void;
        auto swap_chunks() -> void;
        auto init_neighbors(i32 x, i32 z) -> void;

        std::unordered_map<chunk::Chunk *, std::shared_ptr<chunk::Chunk>> chunks;
        std::unordered_map<u32, chunk::Chunk *> active_chunks;
        std::unordered_map<u32, chunk::Chunk *> queued_chunks;

        std::vector<std::pair<u32, chunk::Chunk *>> active_chunks_vec;

        glm::vec2 current_root   = {0.0F, 0.0F};
        glm::vec2 new_root = {0.0F, 0.0F};
        std::mutex mutex;
        std::atomic<bool> queue_ready    = false;

        PlatformState platform_state;
    };
}


#endif //OPENGL_3D_ENGINE_PLATFORM_H
