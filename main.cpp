#include "globalstate.h"

auto main() -> i32 {
    auto fun = [](GlobalGameState &globalState) -> void {
        if (glfwGetKey(globalState._window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(globalState._window, true);

        if (glfwGetKey(globalState._window, GLFW_KEY_W) == GLFW_PRESS)
            globalState._camera->ProcessKeyboard(Camera::FORWARD, static_cast<f32>(globalState._deltaTime));

        if (glfwGetKey(globalState._window, GLFW_KEY_S) == GLFW_PRESS)
            globalState._camera->ProcessKeyboard(Camera::BACKWARD, static_cast<f32>(globalState._deltaTime));

        if (glfwGetKey(globalState._window, GLFW_KEY_A) == GLFW_PRESS)
            globalState._camera->ProcessKeyboard(Camera::LEFT, static_cast<f32>(globalState._deltaTime));

        if (glfwGetKey(globalState._window, GLFW_KEY_D) == GLFW_PRESS)
            globalState._camera->ProcessKeyboard(Camera::RIGHT, static_cast<f32>(globalState._deltaTime));

        if (glfwGetKey(globalState._window, GLFW_KEY_SPACE) == GLFW_PRESS)
            globalState._camera->ProcessKeyboard(Camera::UP, static_cast<f32>(globalState._deltaTime));

        if (glfwGetKey(globalState._window, GLFW_KEY_C) == GLFW_PRESS)
            globalState._camera->ProcessKeyboard(Camera::DOWN, static_cast<f32>(globalState._deltaTime));
    };

    auto globalState = GlobalGameState {};
    globalState._platform->init();

    u64 fps;
    u64 frames = 0;
    f64 step = glfwGetTime();

    while (!glfwWindowShouldClose(globalState._window)) {
        globalState._currentFrame = glfwGetTime();
        globalState._deltaTime    = globalState._currentFrame - globalState._lastFrame;
        globalState._lastFrame    = globalState._currentFrame;

        if ((globalState._lastFrame - step) >= 1.0) {
            fps = static_cast<u32>(static_cast<f32>(frames) / (globalState._lastFrame - step));
            std::cout << std::string(std::to_string(fps)) << "\n";

            step = globalState._lastFrame;
            frames = 0;
        }
        else {
            ++frames;
        }

        fun(globalState);

        // auto t_start = std::chrono::high_resolution_clock::now();

        globalState._platform->tick();

        // auto t_end = std::chrono::high_resolution_clock::now();
        // std::cout << std::chrono::duration<double, std::milli>(t_end-t_start).count() << std::endl;

        globalState._renderer->updateBuffer();
        globalState._renderer->draw(0);

        glfwSwapBuffers(globalState._window);
        glfwPollEvents();
    }

    glfwTerminate();
    exit(EXIT_SUCCESS);
}