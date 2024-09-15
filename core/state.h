//
// Created by Luis Ruisinger on 26.08.24.
//

#ifndef OPENGL_3D_ENGINE_STATE_H
#define OPENGL_3D_ENGINE_STATE_H

#include "level/model/voxel.h"
#include "memory/linear_allocator.h"
#include "memory/arena_allocator.h"

#include "threading/thread_pool.h"

namespace core::rendering::renderer {
    class Renderer;
}

namespace core::level::chunk::chunk_renderer {
    class ChunkRenderer;
}

namespace core::level::platform {
    class Platform;
}

namespace util::player {
    class Player;
}

namespace util::sun {
    class Sun;
}

namespace core::state {
    struct State {
        threading::thread_pool::Tasksystem<> &render_pool;
        threading::thread_pool::Tasksystem<> &chunk_tick_pool;
        threading::thread_pool::Tasksystem<> &normal_tick_pool;

        rendering::renderer::Renderer &renderer;
        level::platform::Platform &platform;

        // TODO: remove this and add ECS later
        util::player::Player &player;
        util::sun::Sun &sun;
    };
}

#endif //OPENGL_3D_ENGINE_STATE_H
