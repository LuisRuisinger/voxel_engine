//
// Created by Luis Ruisinger on 12.04.24.
//

#ifndef OPENGL_3D_ENGINE_GLOBALSTATE_H
#define OPENGL_3D_ENGINE_GLOBALSTATE_H

#include <iostream>

#include "rendering/shader.h"
#include "camera/camera.h"
#include "util/aliases.h"
#include "util/stb_image.h"
#include "level/Model/mesh.h"
#include "rendering/renderer.h"
#include "level/Octree/octree.h"
#include "level/presenter.h"
#include "threading/thread_pool.h"
#include "threading/scheduled_executor.h"
#include "core/io/key_mapping.h"
#include "core/io/window_handler.h"

#define DEFAULT_VIEW             \
    glm::vec3(0.0f, 2.5f, 0.0f), \
    glm::vec3(0.0f, 1.0f, 0.0f), \
    YAW,                         \
    PITCH

class Engine {
public:
    static auto init() -> void {
        DEBUG_LOG("Engine init");
        auto window = Engine::window_handler.init().unwrap();

        DEBUG_LOG("Init framebuffer size callbacks");
        Engine::window_handler.add_framebuffer_size_callback(0, std::move([&]() -> void {
            Engine::camera.setFrustumAspect(
                    static_cast<f32>(window_handler.width / window_handler.height));
        }));

        Engine::window_handler.add_framebuffer_size_callback(1, std::move([&]() -> void {
            Engine::renderer.update_projection_matrix(
                    window_handler.width, window_handler.height);
        }));

        DEBUG_LOG("Init cursor position callbacks");
        Engine::window_handler.add_cursor_position_callback(0, std::move([&]() -> void {
            Engine::camera.ProcessMouseMovement(
                    window_handler.xpos, window_handler.ypos);
        }));

        DEBUG_LOG("Init scheduled executor callbacks")
        Engine::executor.enqueue_detach(std::move([&]() -> void {
            Engine::presenter.tick(
                    Engine::chunk_pool, Engine::camera);
        }));

        DEBUG_LOG("Init renderer");
        Engine::renderer.init_ImGui(window);
        Engine::renderer.init_shaders();
        Engine::renderer.init_pipeline();
        Engine::renderer.update_projection_matrix(
                Engine::window_handler.width, Engine::window_handler.height);
        DEBUG_LOG("Engine init finished");
    }

    static auto run() -> void {
        auto window = Engine::window_handler.get_window().unwrap();

        while (!glfwWindowShouldClose(window)) {
            Engine::time = glfwGetTime();
            Engine::delta_time = Engine::time - Engine::last_frame;
            Engine::last_frame = time;

            core::io::key_mapping::parse_input(
                    window, &Engine::camera, Engine::delta_time);

            Engine::renderer.prepare_frame(Engine::camera);
            Engine::presenter.frame(Engine::render_pool, Engine::camera);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

    static auto shutdown() -> void {
        glfwTerminate();
    }

private:

    // GLFW
    static core::io::window_handler::WindowHandler window_handler;

    // allocator
    static core::memory::arena_allocator::ArenaAllocator allocator;

    // engine
    static core::camera::perspective::Camera camera;
    static core::rendering::Renderer renderer;
    static core::level::Platform platform;
    static core::level::presenter::Presenter presenter;
    // static GLFWwindow *window;

    // threading
    static core::threading::task_system::Tasksystem<> render_pool;
    static core::threading::task_system::Tasksystem<> chunk_pool;
    static core::threading::executor::ScheduledExecutor<> executor;

    // frames
    static f64 delta_time;
    static f64 last_frame;
    static f64 time;
};

decltype(Engine::window_handler) Engine::window_handler {                                      };
decltype(Engine::allocator)      Engine::allocator      {                                      };
decltype(Engine::camera)         Engine::camera         { DEFAULT_VIEW                         };
decltype(Engine::renderer)       Engine::renderer       {                                      };
decltype(Engine::presenter)      Engine::presenter      { Engine::renderer, &Engine::allocator };
decltype(Engine::render_pool)    Engine::render_pool    {                                      };
decltype(Engine::chunk_pool)     Engine::chunk_pool     {                                      };
decltype(Engine::executor)       Engine::executor       {                                      };
decltype(Engine::delta_time)     Engine::delta_time     { 0.0F                                 };
decltype(Engine::last_frame)     Engine::last_frame     { 0.0F                                 };
decltype(Engine::time)           Engine::time           { 0.0F                                 };




#endif //OPENGL_3D_ENGINE_GLOBALSTATE_H
