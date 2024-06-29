//
// Created by Luis Ruisinger on 30.05.24.
//

#include <iostream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "interface.h"
#include "../util/aliases.h"

namespace core::rendering::interface {

    static std::chrono::microseconds render_time = std::chrono::microseconds{0};
    static size_t vertices = 0;
    static glm::vec3 camera = {};
    static u64 fps;
    static u64 frames = 0;
    static f64 step = glfwGetTime();
    static f64 currentFrame = 0;
    static f64 deltaTime = 0;
    static f64 lastFrame = 0;

    auto init(GLFWwindow *window) -> void {
        IMGUI_CHECKVERSION();

        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330");

        currentFrame = glfwGetTime();
        deltaTime    = currentFrame - lastFrame;
        lastFrame    = currentFrame;
    }

    auto update() -> void {
        currentFrame = glfwGetTime();
        deltaTime    = currentFrame - lastFrame;
        lastFrame    = currentFrame;

        if ((lastFrame - step) >= 1.0) {
            fps  = static_cast<u32>(static_cast<f32>(frames) / (lastFrame - step));
            step = lastFrame;
            frames = 0;
        }
        else {
            ++frames;
        }
    }

    auto set_vertices_count(size_t count) -> void {
        vertices = count;
    };

    auto set_render_time(std::chrono::microseconds interval) -> void {
        render_time = interval;
    }

    auto set_camera_pos(glm::vec3 ref) -> void {
        camera = ref;
    }

    auto render() -> void {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        auto render_time_str = std::to_string(render_time.count());
        if (render_time_str.length() > 3)
            render_time_str.insert(render_time_str.length() - 3, ".");
        else
            render_time_str = "0." + render_time_str;

        ImGui::Begin("Debug");
        ImGui::Text("vertices: %s",    std::to_string(vertices).c_str());
        ImGui::Text("platform: %s ms", render_time_str.c_str());
        ImGui::Text("fps:      %s",    std::to_string(fps).c_str());
        ImGui::Text("\n------------------\n\n");
        ImGui::Text("camera:   %.1f %.1f %.1f", camera.x, camera.y, camera.z);
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
}