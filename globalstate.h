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
#include "threading/ticked_executor.h"

struct GlobalGameState {

    // frames
    f64 _deltaTime = 0.0;
    f64 _lastFrame = 0.0;
    f64 _currentFrame = 0.0;

    // engine
    std::shared_ptr<core::camera::perspective::Camera> _camera;
    core::rendering::Renderer                          _renderer;
    core::level::presenter::Presenter                  _presenter;

    // threading
    core::threading::Tasksystem<> _threadPool;
    core::threading::ticked_executor::TickedExecutor<20> _ticked_executor;

    GLFWwindow *_window;

    GlobalGameState()
        : _camera{std::make_shared<core::camera::perspective::Camera>(
                glm::vec3(0.0f, 2.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), YAW, PITCH)}
        , _renderer{_camera}
        , _presenter{_renderer}
        , _window{const_cast<GLFWwindow *>(_renderer.getWindow())}
        , _ticked_executor{*_camera.get()}
    {
        _ticked_executor.attach(static_cast<util::observer::Observer *>(&_presenter));
    }

    ~GlobalGameState() = default;
};


#endif //OPENGL_3D_ENGINE_GLOBALSTATE_H
