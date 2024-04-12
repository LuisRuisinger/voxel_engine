#include "globalstate.h"

auto globalState = GlobalGameState {};

auto processInput(GLFWwindow *window) -> void;

auto main() -> i32 {
    globalState.platform->init();

    u64 fps;
    u64 frames = 0;
    f64 currentFrame;
    f64 step = glfwGetTime();

    while (!glfwWindowShouldClose(globalState.window)) {
        currentFrame = glfwGetTime();

        globalState.deltaTime = currentFrame - globalState.lastFrame;
        globalState.lastFrame = currentFrame;

        if ((globalState.lastFrame - step) >= 1.0) {
            fps = static_cast<u32>(static_cast<f32>(frames) / (globalState.lastFrame - step));
            std::cout << std::string(std::to_string(fps)) << "\n";

            step = globalState.lastFrame;
            frames = 0;
        }
        else {
            ++frames;
        }

        processInput(globalState.window);

        globalState.platform->tick(*globalState.camera);
        globalState.renderer->updateBuffer();
        globalState.renderer->draw({0, 0, 0}, 0);

        glfwSwapBuffers(globalState.window);
        glfwPollEvents();
    }

    glfwTerminate();
    exit(EXIT_SUCCESS);
}

auto processInput(GLFWwindow *window) -> void {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        globalState.camera->ProcessKeyboard(Camera::FORWARD, static_cast<f32>(globalState.deltaTime));

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        globalState.camera->ProcessKeyboard(Camera::BACKWARD, static_cast<f32>(globalState.deltaTime));

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        globalState.camera->ProcessKeyboard(Camera::LEFT, static_cast<f32>(globalState.deltaTime));

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        globalState.camera->ProcessKeyboard(Camera::RIGHT, static_cast<f32>(globalState.deltaTime));

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        globalState.camera->ProcessKeyboard(Camera::UP, static_cast<f32>(globalState.deltaTime));

    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
        globalState.camera->ProcessKeyboard(Camera::DOWN, static_cast<f32>(globalState.deltaTime));
}