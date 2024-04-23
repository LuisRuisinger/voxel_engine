//
// Created by Luis Ruisinger on 12.04.24.
//

#ifndef OPENGL_3D_ENGINE_GLOBALSTATE_H
#define OPENGL_3D_ENGINE_GLOBALSTATE_H

#include <iostream>

#include "Shader.h"
#include "camera.h"
#include "global.h"
#include "stb_image.h"
#include "Level/Model/Mesh.h"
#include "Rendering/Renderer.h"
#include "Level/Octree/Octree.h"
#include "Level/Platform.h"

struct GlobalGameState {

    // ------
    // frames

    f64 _deltaTime;
    f64 _lastFrame;

    // ------
    // engine

    std::shared_ptr<Camera::Camera>     _camera;
    std::shared_ptr<Renderer::Renderer> _renderer;
    std::shared_ptr<Platform::Platform> _platform;

    GLFWwindow *_window;

    GlobalGameState()
        : _deltaTime{0.0}
        , _lastFrame{0.0}
        , _camera{std::make_shared<Camera::Camera>(vec3f(0.0f, 2.5f, 0.0f), vec3f(0.0f, 1.0f, 0.0f),YAW, PITCH)}
        , _renderer{std::make_unique<Renderer::Renderer>(_camera)}
        , _platform{std::make_unique<Platform::Platform>(*(_renderer))}
        , _window{const_cast<GLFWwindow *>(_renderer->getWindow())}
    {}

    ~GlobalGameState() = default;
};


#endif //OPENGL_3D_ENGINE_GLOBALSTATE_H
