//
// Created by Luis Ruisinger on 23.08.24.
//

#include "window_handler.h"

#define DEFAULT_WIDTH  1920
#define DEFAULT_HEIGHT 1080
#define WINDOW         "Window"

namespace core::io::window_handler {
    WindowHandler::WindowHandler()
        : width  { DEFAULT_WIDTH  },
          height { DEFAULT_HEIGHT }
    {}

    auto WindowHandler::init() -> Result<GLFWwindow *, GLFWerror> {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

        this->window = glfwCreateWindow(
                this->width, this->height, WINDOW, nullptr, nullptr);

        if (!this->window)
            throw std::runtime_error{"ERR::RENDERER::INIT::WINDOW"};

        glfwMakeContextCurrent(this->window);
        glfwSetWindowUserPointer(this->window, this);

        auto framebuffer_size_callback = [](GLFWwindow *window, i32 width, i32 height) -> void {
            auto self = static_cast<decltype(this)>(glfwGetWindowUserPointer(window));
            self->width  = width;
            self->height = height;

            glViewport(0, 0, self->width, self->height);
            for (const auto &[_, callback] : self->framebuffer_size_callbacks)
                callback();
        };

        glfwSetFramebufferSizeCallback(this->window, std::move(framebuffer_size_callback));
        glfwSwapInterval(0);

        // glad: load all OpenGL function pointers
        if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
            return Err(GLFWerror::TEST);

        glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        auto cursor_position_callback = [](GLFWwindow *window, f64 xpos, f64 ypos) -> void {
            if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)
                return;

            auto self = static_cast<decltype(this)>(glfwGetWindowUserPointer(window));
            self->xpos = xpos;
            self->ypos = ypos;

            for (const auto &[_, callback] : self->cursor_position_callbacks)
                callback();
        };

        glfwSetCursorPosCallback(this->window, std::move(cursor_position_callback));
        return Ok(this->window);
    }

    auto WindowHandler::remove_framebuffer_size_callback(u64 id) -> void {
        this->framebuffer_size_callbacks.erase(id);
    }

    auto WindowHandler::remove_cursor_position_callback(u64 id) -> void {
        this->cursor_position_callbacks.erase(id);
    }

    auto WindowHandler::get_window() -> Result<GLFWwindow *, GLFWerror> {
        // TODO if init throws error
        return Ok(this->window);
    }
}
