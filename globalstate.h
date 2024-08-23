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

#define DEFAULT_VIEW std::make_shared<core::camera::perspective::Camera>( \
    glm::vec3(0.0f, 2.5f, 0.0f),                                          \
    glm::vec3(0.0f, 1.0f, 0.0f),                                          \
    YAW,                                                                  \
    PITCH)

class Engine {
public:
    static auto init() -> void {
        Engine::executor.enqueue_detach(std::move([&]() -> void {
            Engine::presenter.tick(Engine::chunk_pool, *Engine::camera.get());
        }));

        DEBUG_LOG("Engine init finished");
    }

    static auto run() -> void {
        while (!glfwWindowShouldClose(Engine::window)) {
            Engine::time = glfwGetTime();
            Engine::delta_time = Engine::time - Engine::last_frame;
            Engine::last_frame = time;

            core::io::key_mapping::parse_input(Engine::window,
                                               Engine::camera.get(),
                                               Engine::delta_time);

            Engine::renderer.prepare_frame();
            Engine::presenter.frame(Engine::render_pool, *Engine::camera);

            glfwSwapBuffers(Engine::window);
            glfwPollEvents();
        }

        glfwTerminate();
    }

private:
    // allocator
    static core::memory::arena_allocator::ArenaAllocator allocator;

    // engine
    static std::shared_ptr<core::camera::perspective::Camera> camera;
    static core::rendering::Renderer renderer;
    static core::level::presenter::Presenter presenter;
    static GLFWwindow *window;

    // threading
    static core::threading::Tasksystem<> render_pool;
    static core::threading::Tasksystem<> chunk_pool;
    static core::threading::executor::ScheduledExecutor<> executor;

    // frames
    static f64 delta_time;
    static f64 last_frame;
    static f64 time;
};

core::memory::arena_allocator::ArenaAllocator      Engine::allocator   {};
std::shared_ptr<core::camera::perspective::Camera> Engine::camera      { DEFAULT_VIEW };
core::rendering::Renderer                          Engine::renderer    { Engine::camera };
core::level::presenter::Presenter                  Engine::presenter   { Engine::renderer, &Engine::allocator };
GLFWwindow *                                       Engine::window      { const_cast<GLFWwindow *>(Engine::renderer.getWindow()) };
core::threading::Tasksystem<>                      Engine::render_pool {};
core::threading::Tasksystem<>                      Engine::chunk_pool  {};
core::threading::executor::ScheduledExecutor<>     Engine::executor    {};
f64                                                Engine::delta_time  { 0.0F };
f64                                                Engine::last_frame  { 0.0F };
f64                                                Engine::time        { 0.0F };




#endif //OPENGL_3D_ENGINE_GLOBALSTATE_H
