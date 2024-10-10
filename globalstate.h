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
    static auto init() -> void {
        DEBUG_LOG("Engine init");
        auto window = Engine::window_handler.init();

        DEBUG_LOG("Init framebuffer size callbacks");
        Engine::window_handler.add_framebuffer_size_callback(
                0, std::move([&](std::pair<i32, i32> &ref) -> void {
                    auto &camera = Engine::state.player.get_camera();
                    camera.set_frustum_aspect(
                            static_cast<f32>(ref.first) / static_cast<f32>(ref.second));
                    camera.set_projection_matrix(ref.first, ref.second);
                }));

        Engine::window_handler.add_framebuffer_size_callback(
                1, std::move([&](std::pair<i32, i32> &ref) -> void {
                    Engine::renderer.resize(ref.first, ref.second);
                }));

        DEBUG_LOG("Init cursor position callbacks");
        Engine::window_handler.add_cursor_position_callback(
                0, std::move([&](std::pair<f32, f32> &ref) -> void {
                    auto &camera = Engine::state.player.get_camera();
                    camera.rotate_camera(ref.first, ref.second);
                }));

        DEBUG_LOG("Init key callbacks");
        Engine::window_handler.add_key_callback(
                0, [&](std::pair<i32, i32> &ref) -> void {
                    Engine::key_map.handle_event(ref);
                });

        DEBUG_LOG("Init renderer")
        Engine::renderer.add_sub_renderer(
                core::rendering::renderer::CHUNK_RENDERER,
                reinterpret_cast<
                    util::renderable::Renderable<
                        util::renderable::BaseInterface> *>(&Engine::chunk_renderer));

        /*
        Engine::renderer.add_sub_renderer(
                core::rendering::renderer::SKYBOX_RENDERER,
                reinterpret_cast<
                    util::renderable::Renderable<
                        util::renderable::BaseInterface> *>(&Engine::skybox));
                        */

        DEBUG_LOG("Init extra key_map calls");
        Engine::key_map.add_callback(
                core::opengl::opengl_key_map::Action::ON_PRESSED,
                Keymap::LEFT_ALT,
                [&]() -> void {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                });

        Engine::key_map.add_callback(
                core::opengl::opengl_key_map::Action::ON_RELEASE,
                Keymap::LEFT_ALT,
                [&]() -> void {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                });

        Engine::key_map.add_callback(
                core::opengl::opengl_key_map::Action::ON_RELEASE,
                Keymap::KEY_ESCAPE,
                [&]() -> void {
                    glfwSetWindowShouldClose(window, true);
                });

        DEBUG_LOG("Init scheduled executor callbacks")
        Engine::executor.enqueue_detach(std::move([&]() -> void {
            Engine::time = glfwGetTime();
            Engine::delta_time = Engine::time - Engine::last_frame;
            Engine::last_frame = time;
            Engine::state.player.update_delta_time(Engine::delta_time);

            Engine::state.player.tick(state);
            Engine::state.platform.tick(state);
            Engine::sun.tick(state);

            Engine::key_map.run_repeat();
        }));

        DEBUG_LOG("Init tile_manager");
        core::level::tiles::tile_manager::setup(Engine::tile_manager);

        DEBUG_LOG("Init renderer");
        Engine::renderer.init_ImGui(window);
        Engine::renderer.init_pipeline();

        DEBUG_LOG("Engine init finished");
    }

    static auto run() -> void {
        auto window = Engine::window_handler.get_window();

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            Engine::renderer.prepare_frame(Engine::state);

            auto t_start = std::chrono::high_resolution_clock::now();
            Engine::platform.update(Engine::state);
            auto t_end = std::chrono::high_resolution_clock::now();
            auto t_diff = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start);

            core::rendering::interface::set_render_time(t_diff);
            Engine::renderer.frame(Engine::state);

            glfwSwapBuffers(window);
        }
    }

    static auto shutdown() -> void {
        glfwTerminate();
    }

private:

    // opengl
    static core::opengl::opengl_window::OpenGLWindow window_handler;
    static core::opengl::opengl_key_map::OpenGLKeyMap key_map;

    // memory
    static core::memory::arena_allocator::ArenaAllocator allocator;

    // concurrency
    static core::threading::executor::ScheduledExecutor<> executor;
    static core::threading::thread_pool::Tasksystem<> render_pool;
    static core::threading::thread_pool::Tasksystem<> chunk_tick_pool;
    static core::threading::thread_pool::Tasksystem<> normal_tick_pool;

    // tile system
    static core::level::tiles::tile_manager::TileManager tile_manager;

    // rendering
    static core::rendering::renderer::Renderer renderer;
    static core::level::chunk::chunk_renderer::ChunkRenderer chunk_renderer;

    // platform
    static core::level::platform::Platform platform;

    // utils
    static util::player::Player player;
    static util::sun::Sun sun;
    static core::state::State state;

    // frames
    static f64 delta_time;
    static f64 last_frame;
    static f64 time;
};

decltype(Engine::window_handler)   Engine::window_handler   {                           };
decltype(Engine::key_map)          Engine::key_map          {                           };
decltype(Engine::allocator)        Engine::allocator        {                           };
decltype(Engine::renderer)         Engine::renderer         {                           };
decltype(Engine::executor)         Engine::executor         {                           };
decltype(Engine::render_pool)      Engine::render_pool      {                           };
decltype(Engine::chunk_tick_pool)  Engine::chunk_tick_pool  {                           };
decltype(Engine::normal_tick_pool) Engine::normal_tick_pool {                           };
decltype(Engine::tile_manager)     Engine::tile_manager     {                           };
decltype(Engine::chunk_renderer)   Engine::chunk_renderer   { &Engine::allocator        };
decltype(Engine::platform)         Engine::platform         {                           };
decltype(Engine::player)           Engine::player           { Engine::key_map           };
decltype(Engine::sun)              Engine::sun              {                           };
decltype(Engine::state)            Engine::state            { Engine::render_pool,
                                                              Engine::chunk_tick_pool,
                                                              Engine::normal_tick_pool,
                                                              Engine::renderer,
                                                              Engine::platform,
                                                              Engine::tile_manager,
                                                              Engine::player,
                                                              Engine::sun               };
decltype(Engine::delta_time)       Engine::delta_time       { 0.0F                      };
decltype(Engine::last_frame)       Engine::last_frame       { 0.0F                      };
decltype(Engine::time)             Engine::time             { 0.0F                      };




#endif //OPENGL_3D_ENGINE_GLOBALSTATE_H
