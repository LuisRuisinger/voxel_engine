//
// Created by Luis Ruisinger on 12.04.24.
//

#ifndef OPENGL_3D_ENGINE_GLOBALSTATE_H
#define OPENGL_3D_ENGINE_GLOBALSTATE_H

#include "core/rendering/renderer.h"
#include "core/level/chunk/chunk_renderer.h"

#include "core/threading/thread_pool.h"
#include "core/threading/scheduled_executor.h"

#include "core/level/platform.h"

#include "core/opengl/opengl_window.h"
#include "core/opengl/opengl_key_map.h"

#include "util/stb_image.h"
#include "util/player.h"

#include "core/level/tiles/tile_manager.h"
#include "util/sun.h"

class Engine {
public:
    auto init() -> void {
        DEBUG_LOG("Engine init");
        auto window = this->window_handler.init();

        DEBUG_LOG("Init framebuffer size callbacks");
        this->window_handler.add_framebuffer_size_callback(
                0, std::move([&](std::pair<i32, i32> &ref) -> void {
                    auto &camera = this->state.player.get_camera();
                    camera.set_frustum_aspect(
                            static_cast<f32>(ref.first) / static_cast<f32>(ref.second));
                    camera.set_projection_matrix(ref.first, ref.second);
                }));

        this->window_handler.add_framebuffer_size_callback(
                1, std::move([&](std::pair<i32, i32> &ref) -> void {
                    this->renderer.resize(ref.first, ref.second);
                }));

        DEBUG_LOG("Init cursor position callbacks");
        this->window_handler.add_cursor_position_callback(
                0, std::move([&](std::pair<f32, f32> &ref) -> void {
                    auto &camera = this->state.player.get_camera();
                    camera.rotate_camera(ref.first, ref.second);
                }));

        DEBUG_LOG("Init key callbacks");
        this->window_handler.add_key_callback(
                0, [&](std::pair<i32, i32> &ref) -> void {
                    this->key_map.handle_event(ref);
                });

        DEBUG_LOG("Init renderer")
        this->renderer.add_sub_renderer(
                core::rendering::renderer::CHUNK_RENDERER,
                reinterpret_cast<
                    util::renderable::Renderable<
                        util::renderable::BaseInterface> *>(&this->chunk_renderer));

        this->renderer.add_sub_renderer(
                core::rendering::renderer::WATER_RENDERER,
                reinterpret_cast<
                    util::renderable::Renderable<
                        util::renderable::BaseInterface> *>(&this->water_renderer));

        DEBUG_LOG("Init extra key_map calls");
        this->key_map.add_callback(
                core::opengl::opengl_key_map::Action::ON_PRESSED,
                Keymap::LEFT_ALT,
                [&]() -> void {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                });

        this->key_map.add_callback(
                core::opengl::opengl_key_map::Action::ON_RELEASE,
                Keymap::LEFT_ALT,
                [&]() -> void {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                });

        this->key_map.add_callback(
                core::opengl::opengl_key_map::Action::ON_RELEASE,
                Keymap::KEY_ESCAPE,
                [&]() -> void {
                    glfwSetWindowShouldClose(window, true);
                });

        DEBUG_LOG("Init renderer");
        this->renderer.init_ImGui(window);
        this->renderer.init_pipeline();

        DEBUG_LOG("Init tile_manager");
        core::level::tiles::tile_manager::setup(core::level::tiles::tile_manager::tile_manager);

        DEBUG_LOG("Init scheduled executor callbacks")
        this->executor.enqueue_detach(std::move([&]() -> void {
            this->time = glfwGetTime();
            this->delta_time = this->time - this->last_frame;
            this->last_frame = time;
            this->state.player.update_delta_time(this->delta_time);

            this->state.tick(state);
            this->state.player.tick(state);
            this->state.platform.tick(state);

            // the sun tick must happen after player movement because the depth map orientation
            // and size is calculated using the player position / player camera orientation
            // and the now changed sun position and orientation
            this->sun.tick(state);

            this->key_map.run_repeat();
        }));
    }

    auto run() -> void {
        auto window = this->window_handler.get_window();

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            this->renderer.prepare_frame(this->state);

            auto t_start = std::chrono::high_resolution_clock::now();
            this->platform.update(this->state);
            auto t_end = std::chrono::high_resolution_clock::now();
            auto t_diff = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start);

            core::rendering::interface::set_render_time(t_diff);
            this->renderer.frame(this->state);

            glfwSwapBuffers(window);
        }
    }

    auto shutdown() -> void {
        glfwTerminate();
    }

    Engine()
        : window_handler   {                         },
          key_map          {                         },
          allocator        {                         },
          renderer         {                         },
          executor         {                         },
          render_pool      {                         },
          chunk_tick_pool  {                         },
          normal_tick_pool {                         },
          chunk_renderer   { &this->allocator        },
          water_renderer   { &this->allocator        },
          platform         {                         },
          player           { this->key_map },
          sun              {                         },
          state            { this->render_pool,
                             this->chunk_tick_pool,
                             this->normal_tick_pool,
                             this->renderer,
                             this->platform,
                             this->player,
                             this->sun               }
    {}


private:

    // opengl
    core::opengl::opengl_window::OpenGLWindow window_handler;
    core::opengl::opengl_key_map::OpenGLKeyMap key_map;

    // memory
    core::memory::arena_allocator::ArenaAllocator allocator;

    // concurrency
    core::threading::executor::ScheduledExecutor<> executor;
    core::threading::thread_pool::Tasksystem<> render_pool;
    core::threading::thread_pool::Tasksystem<> chunk_tick_pool;
    core::threading::thread_pool::Tasksystem<> normal_tick_pool;

    // tile system
    // static core::level::tiles::tile_manager::TileManager tile_manager;

    // rendering
    core::rendering::renderer::Renderer renderer;
    core::level::chunk::chunk_renderer::ChunkRenderer chunk_renderer;
    core::level::chunk::chunk_renderer::ChunkRenderer water_renderer;

    // platform
    core::level::platform::Platform platform;

    // utils
    util::player::Player player;
    util::sun::Sun sun;
    core::state::State state;

    // frames
    f64 delta_time;
    f64 last_frame;
    f64 time;
};

#endif //OPENGL_3D_ENGINE_GLOBALSTATE_H
