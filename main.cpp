#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <filesystem>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "camera.h"

#include "global.h"
#include "stb_image.h"
#include "Level/Model/Mesh.h"
#include "Rendering/Renderer.h"
#include "Level/Octree/Octree.h"
#include "Level/Platform.h"

auto processInput(GLFWwindow *window) -> void;
auto mouse_callback(GLFWwindow *window, f64 xposIn, f64 yposIn) -> void;

u32 SCR_WIDTH = 1800;
u32 SCR_HEIGHT = 1200;

f64 deltaTime = 0.0f; // Time between current frame and last frame
f64 lastFrame = 0.0f; // Time of last frame

f32 lastX = (f32) SCR_WIDTH / 2.0f;
f32 lastY = (f32) SCR_HEIGHT / 2.0f;
bool firstMouse = true;
bool pressed = false;

auto sharedCam = std::make_shared<Camera::Camera>(vec3f(0.0f, 2.5f, 0.0f),
                                                  vec3f(0.0f, 1.0f, 0.0f),
                                                  YAW, PITCH);
auto camera = sharedCam.get();

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Farlands", nullptr, nullptr);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();

        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, i32 width, i32 height) {
        SCR_WIDTH  = width;
        SCR_HEIGHT = height;
        glViewport(0, 0, width, height);

        sharedCam->setFrustumAspect(((f32) width) / ((f32) height));
    });

    glfwSwapInterval(0);

    // glad: load all OpenGL function pointers
    // ---------------------------------------

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        exit(EXIT_FAILURE);
    }

    // build and compile our shader program
    // ------------------------------------

    Shader shader("../shaders/vertex_shader.glsl",
                  "../shaders/fragment_shader.glsl");

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

    Renderer::Renderer renderer{sharedCam};
    Platform::Platform platform{renderer};
    platform.init();

    u32 fps;
    u32 frames = 0;
    f32 currentFrame;
    f32 step = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if ((lastFrame - step) >= 1.0) {
            fps = (uint32_t) (frames / (lastFrame - step));
            std::cout << std::string(std::to_string(fps)) << "\n";

            step = lastFrame;
            frames = 0;
        }
        else {
            ++frames;
        }

        processInput(window);

        auto cpos = sharedCam->getCameraFront();
        //std::cout << cpos.x << " " << cpos.y << " " << cpos.z << '\n';

        //auto start = std::chrono::high_resolution_clock::now();

        platform.tick(*sharedCam);
        renderer.updateBuffer();

        //auto end = std::chrono::high_resolution_clock::now();
        //auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        //std::cout << duration.count() << " mus" << std::endl;

        glm::mat4 projection =
                glm::perspective(glm::radians(45.0f),
                                 ((f32) SCR_WIDTH) / ((f32) SCR_HEIGHT),
                                 0.1f,
                                 ((f32) (RENDER_RADIUS * 2) * CHUNK_SIZE));

        renderer.draw(shader, projection, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    exit(EXIT_SUCCESS);
}

auto processInput(GLFWwindow *window) -> void {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera->ProcessKeyboard(Camera::FORWARD, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera->ProcessKeyboard(Camera::BACKWARD, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera->ProcessKeyboard(Camera::LEFT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera->ProcessKeyboard(Camera::RIGHT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera->ProcessKeyboard(Camera::UP, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
        camera->ProcessKeyboard(Camera::DOWN, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS && !pressed) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        pressed = true;
    }

    else if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS && pressed) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        pressed = false;
    }
}

auto mouse_callback(GLFWwindow *window, f64 xpos, f64 ypos) -> void {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    f32 xoffset = xpos - lastX;
    f32 yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera->ProcessMouseMovement(xoffset, yoffset, true);
}