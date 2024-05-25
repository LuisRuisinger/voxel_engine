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
#include "level/platform.h"
#include "threading/thread_pool.h"

struct GlobalGameState {

    // frames
    f64 _deltaTime;
    f64 _lastFrame;
    f64 _currentFrame;

    // engine
    std::shared_ptr<core::camera::perspective::Camera> _camera;
    std::shared_ptr<core::rendering::Renderer>         _renderer;
    std::shared_ptr<core::level::Platform>             _platform;

    // threadpool
    std::unique_ptr<core::threading::Tasksystem<>> _threadPool;

    GLFWwindow *_window;

    GlobalGameState()
        : _currentFrame{0.0}
        , _deltaTime{0.0}
        , _lastFrame{0.0}
        , _camera{std::make_shared<core::camera::perspective::Camera>(
                glm::vec3(0.0f, 2.5f, 0.0f),
                glm::vec3(0.0f, 1.0f, 0.0f),
                YAW,
                PITCH)}
        , _renderer{std::make_unique<core::rendering::Renderer>(_camera)}
        , _platform{std::make_unique<core::level::Platform>(*(_renderer))}
        , _threadPool{std::make_unique<core::threading::Tasksystem<>>()}
        , _window{const_cast<GLFWwindow *>(_renderer->getWindow())}
    {}

    ~GlobalGameState() = default;
};


#endif //OPENGL_3D_ENGINE_GLOBALSTATE_H
