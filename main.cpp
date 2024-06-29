#include "globalstate.h"
#include "util/singleton.h"
#include "util/log.h"
#include "util/assert.h"

auto main() -> i32 {
    auto fun = [](GlobalGameState &globalState) -> void {

        if (glfwGetKey(globalState._window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
            glfwSetInputMode(globalState._window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        else if (glfwGetKey(globalState._window, GLFW_KEY_LEFT_ALT) == GLFW_RELEASE) {
            glfwSetInputMode(globalState._window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

            if (glfwGetKey(globalState._window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(globalState._window, true);

            if (glfwGetKey(globalState._window, GLFW_KEY_W) == GLFW_PRESS)
                globalState._camera->ProcessKeyboard(core::camera::FORWARD, static_cast<f32>(globalState._deltaTime));

            if (glfwGetKey(globalState._window, GLFW_KEY_S) == GLFW_PRESS)
                globalState._camera->ProcessKeyboard(core::camera::BACKWARD, static_cast<f32>(globalState._deltaTime));

            if (glfwGetKey(globalState._window, GLFW_KEY_A) == GLFW_PRESS)
                globalState._camera->ProcessKeyboard(core::camera::LEFT, static_cast<f32>(globalState._deltaTime));

            if (glfwGetKey(globalState._window, GLFW_KEY_D) == GLFW_PRESS)
                globalState._camera->ProcessKeyboard(core::camera::RIGHT, static_cast<f32>(globalState._deltaTime));

            if (glfwGetKey(globalState._window, GLFW_KEY_SPACE) == GLFW_PRESS)
                globalState._camera->ProcessKeyboard(core::camera::UP, static_cast<f32>(globalState._deltaTime));

            if (glfwGetKey(globalState._window, GLFW_KEY_C) == GLFW_PRESS)
                globalState._camera->ProcessKeyboard(core::camera::DOWN, static_cast<f32>(globalState._deltaTime));
        }
    };

    LOG("Init");
    auto globalState = GlobalGameState {};

    u64 fps;
    u64 frames = 0;
    f64 step = glfwGetTime();

    LOG("Init finished");

    while (!glfwWindowShouldClose(globalState._window)) {
        globalState._currentFrame = glfwGetTime();
        globalState._deltaTime    = globalState._currentFrame - globalState._lastFrame;
        globalState._lastFrame    = globalState._currentFrame;

        fun(globalState);

        //globalState._presenter.tick(globalState._threadPool, *globalState._camera);

        globalState._renderer.prepare_frame();
        globalState._presenter.frame(globalState._threadPool, *globalState._camera);

        glfwSwapBuffers(globalState._window);
        glfwPollEvents();
    }

    glfwTerminate();
    exit(EXIT_SUCCESS);
}