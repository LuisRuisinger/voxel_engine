//
// Created by Luis Ruisinger on 23.08.24.
//

#ifndef OPENGL_3D_ENGINE_KEY_MAPPING_H
#define OPENGL_3D_ENGINE_KEY_MAPPING_H

#include "../../camera/camera.h"
#include "GLFW/glfw3.h"

namespace core::io::key_mapping {
    auto parse_input(GLFWwindow *window,
                     camera::perspective::Camera *camera,
                     f64 delta_time) -> void {
        if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        else if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_RELEASE) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(window, true);

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                camera->ProcessKeyboard(core::camera::FORWARD, static_cast<f32>(delta_time));

            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                camera->ProcessKeyboard(core::camera::BACKWARD, static_cast<f32>(delta_time));

            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                camera->ProcessKeyboard(core::camera::LEFT, static_cast<f32>(delta_time));

            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                camera->ProcessKeyboard(core::camera::RIGHT, static_cast<f32>(delta_time));

            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
                camera->ProcessKeyboard(core::camera::UP, static_cast<f32>(delta_time));

            if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
                camera->ProcessKeyboard(core::camera::DOWN, static_cast<f32>(delta_time));
        }
    }
}

#endif //OPENGL_3D_ENGINE_KEY_MAPPING_H
