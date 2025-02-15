//
// Created by Luis Ruisinger on 23.08.24.
//

#include "opengl_window.h"
#include "opengl_verify.h"

#include <filesystem>

#define WINDOW "Window"

namespace core::opengl::opengl_window {
    OpenGLWindow::OpenGLWindow() {}

    auto OpenGLWindow::init() -> GLFWwindow * {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
        this->window = glfwCreateWindow(
                DEFAULT_WIDTH, DEFAULT_HEIGHT, WINDOW, nullptr, nullptr);

        if (!this->window)
            throw std::runtime_error{"ERR::RENDERER::INIT::WINDOW"};

        glfwMakeContextCurrent(this->window);
        glfwSetWindowUserPointer(this->window, this);

        auto framebuffer_size_callback = [](GLFWwindow *window, i32 width, i32 height) -> void {
            auto self = static_cast<decltype(this)>(glfwGetWindowUserPointer(window));
            auto resize_token = std::pair { width, height };

            glViewport(0, 0, width, height);
            for (const auto &[_, callback] : self->framebuffer_size_callbacks)
                callback(resize_token);
        };

        glfwSetFramebufferSizeCallback(this->window, std::move(framebuffer_size_callback));
        glfwSwapInterval(Vsync::DISABLED);
        gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
        glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        auto cursor_position_callback = [](GLFWwindow *_window, f64 xpos, f64 ypos) -> void {
            if (glfwGetKey(_window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)
                return;

            auto self = static_cast<decltype(this)>(glfwGetWindowUserPointer(_window));
            auto pos_token = std::pair {
                static_cast<f32>(xpos),
                static_cast<f32>(ypos)
            };

            for (const auto &[_, callback] : self->cursor_position_callbacks)
                callback(pos_token);
        };

        glfwSetCursorPosCallback(this->window, std::move(cursor_position_callback));

        auto key_callback = [](
                GLFWwindow *_window,
                i32 key,
                i32 code,
                i32 action,
                i32 mods) -> void {
            auto self = static_cast<decltype(this)>(glfwGetWindowUserPointer(_window));
            auto key_token = std::pair { action, key };

            for (const auto &[_, callback] : self->key_callbacks)
                callback(key_token);
        };

#ifdef DEBUG
        GLint no_of_extensions = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &no_of_extensions);
        for (auto i = 0; i < no_of_extensions; ++i)
            LOG(std::string { reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i)) });

        GLint max_layers;
        glGetIntegerv (GL_MAX_ARRAY_TEXTURE_LAYERS, &max_layers);
        DEBUG_LOG("max layers", max_layers);

        std::filesystem::path projectRoot = std::filesystem::current_path();
        DEBUG_LOG(projectRoot);
#endif

        glfwSetKeyCallback(this->window, std::move(key_callback));
        return this->window;
    }

    auto OpenGLWindow::remove_framebuffer_size_callback(u64 id) -> void {
        this->framebuffer_size_callbacks.erase(id);
    }

    auto OpenGLWindow::remove_cursor_position_callback(u64 id) -> void {
        this->cursor_position_callbacks.erase(id);
    }

    auto OpenGLWindow::get_window() -> GLFWwindow *{
        return this->window;
    }

    auto OpenGLWindow::set_vsync(Vsync mode) -> void {
        glfwSwapInterval(mode);
    }
}
