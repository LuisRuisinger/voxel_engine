//
// Created by Luis Ruisinger on 30.05.24.
//

#ifndef OPENGL_3D_ENGINE_INTERFACE_H
#define OPENGL_3D_ENGINE_INTERFACE_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <chrono>

#include "glm/vec3.hpp"
#include "../util/aliases.h"

namespace core::rendering::interface {
    auto init(GLFWwindow *) -> void;
    auto update() -> void;
    auto set_vertices_count(size_t count) -> void;
    auto set_camera_pos(glm::vec3) -> void;
    auto set_render_time(std::chrono::microseconds interval) -> void;
    auto add_wait_time(std::chrono::microseconds interval) -> void;
    auto set_draw_calls(u64) -> void;
    auto render() -> void;
};


#endif //OPENGL_3D_ENGINE_INTERFACE_H
