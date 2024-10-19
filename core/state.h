//
// Created by Luis Ruisinger on 26.08.24.
//

#ifndef OPENGL_3D_ENGINE_STATE_H
#define OPENGL_3D_ENGINE_STATE_H

#include "level/model/voxel.h"
#include "memory/linear_allocator.h"
#include "memory/arena_allocator.h"

#include "threading/thread_pool.h"
#include "../util/traits.h"

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

namespace core::level::tiles::tile_manager {
    class TileManager;
}

namespace core::state {
    struct State : public util::traits::Tickable<State> {

        // task systems
        threading::thread_pool::Tasksystem<> &render_pool;
        threading::thread_pool::Tasksystem<> &chunk_tick_pool;
        threading::thread_pool::Tasksystem<> &normal_tick_pool;

        rendering::renderer::Renderer        &renderer;
        level::platform::Platform            &platform;


        util::player::Player                 &player;
        util::sun::Sun                       &sun;

        u32 max_tick_count = 2400;
        u32 current_tick = 0;
        u64 ticks_since_start = 0;

        State(threading::thread_pool::Tasksystem<> &render_pool,
              threading::thread_pool::Tasksystem<> &chunk_tick_pool,
              threading::thread_pool::Tasksystem<> &normal_tick_pool,
              rendering::renderer::Renderer        &renderer,
              level::platform::Platform            &platform,
              util::player::Player                 &player,
              util::sun::Sun                       &sun)
                : render_pool      { render_pool      },
                  chunk_tick_pool  { chunk_tick_pool  },
                  normal_tick_pool { normal_tick_pool },
                  renderer         { renderer         },
                  platform         { platform         },
                  player           { player           },
                  sun              { sun              }
        {}


        auto tick(core::state::State &) -> void {
            this->current_tick = (this->current_tick + 1) % this->max_tick_count;
            ++this->ticks_since_start;
        }
    };
}


#endif //OPENGL_3D_ENGINE_STATE_H
