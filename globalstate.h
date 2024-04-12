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
    f64 deltaTime;
    f64 lastFrame;

    std::shared_ptr<Camera::Camera>     camera;
    std::shared_ptr<Renderer::Renderer> renderer;
    std::shared_ptr<Platform::Platform> platform;

    GLFWwindow *window;

    GlobalGameState()
        : deltaTime{0.0}
        , lastFrame{0.0}
        , camera{std::make_shared<Camera::Camera>(vec3f(0.0f, 2.5f, 0.0f), vec3f(0.0f, 1.0f, 0.0f),YAW, PITCH)}
        , renderer{std::make_unique<Renderer::Renderer>(this->camera)}
        , platform{std::make_unique<Platform::Platform>(*(this->renderer))}
        , window{const_cast<GLFWwindow *>(renderer->getWindow())}
    {}

    ~GlobalGameState() = default;
};


#endif //OPENGL_3D_ENGINE_GLOBALSTATE_H
