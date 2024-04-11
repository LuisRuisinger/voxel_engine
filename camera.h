//
// Created by Luis Ruisinger on 15.02.24.
//

#ifndef OPENGL_3D_ENGINE_CAMERA_H
#define OPENGL_3D_ENGINE_CAMERA_H

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "glad/glad.h"

#include "global.h"
#include "Rendering/Culling.h"

#define YAW 0.0f
#define PITCH 0.0f
#define SPEED 50.0f
#define SENSITIVITY 0.1f

namespace Camera
{
    // Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
    enum Camera_Movement {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
        UP,
        DOWN
    };

    class Camera {
    public:
        Camera(glm::vec3 initPosition, glm::vec3 initUp, float initYaw, float initPitch);
        Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch);

        auto GetViewMatrix() -> glm::mat4;
        auto ProcessKeyboard(Camera_Movement direction, float deltaTime) -> void;
        auto ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch) -> void;

        auto setFrustum(float_t angle, float_t ratio, float_t nearD, float_t farD) -> void;
        auto setFrustumAspect(f32 ratio) -> void;
        auto setFrustumDef(glm::vec3 position, glm::vec3 target, glm::vec3 up) -> void;
        [[nodiscard]] auto inFrustum(glm::vec3 position, uint32_t scale) const -> bool;
        [[nodiscard]] auto inFrustum(glm::vec2 position, uint32_t scale) const -> bool;

        [[nodiscard]] auto getCameraPosition() const -> glm::vec3;
        [[nodiscard]] auto getCameraFront() const ->  glm::vec3;
        [[nodiscard]] auto getCameraMask() const -> u8;

    private:
        void updateCameraVectors();

        // camera Attributes
        glm::vec3 position;
        glm::vec3 front;
        glm::vec3 up;
        glm::vec3 right;
        glm::vec3 worldUp;

        // euler Angles
        float yaw;
        float pitch;

        // camera options
        float movementSpeed;
        float mouseSensitivity;

        //frustum
        Culling::Frustum frustum;

        u8 mask;
    };
}


#endif //OPENGL_3D_ENGINE_CAMERA_H
