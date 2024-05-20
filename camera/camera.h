//
// Created by Luis Ruisinger on 15.02.24.
//

#ifndef OPENGL_3D_ENGINE_CAMERA_H
#define OPENGL_3D_ENGINE_CAMERA_H

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glad/glad.h"

#include "../util/aliases.h"
#include "culling.h"
#include "../util/updateable.h"

#define YAW         0.0F
#define PITCH       0.0F
#define SPEED       50.0F
#define SENSITIVITY 0.1F

namespace core::camera {
    enum Camera_Movement {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
        UP,
        DOWN
    };
}

//
//
//

namespace core::camera::perspective {
    class Camera : util::Updateable {
    public:
        Camera(glm::vec3 initPosition, glm::vec3 initUp, float initYaw, float initPitch);
        Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch);
        Camera(Camera &&other) noexcept = default;
        Camera(Camera &other) = delete;

        ~Camera() = default;

        auto operator=(Camera &&other) noexcept -> Camera & = default;
        auto operator=(Camera &other) -> Camera & = delete;

        auto ProcessKeyboard(Camera_Movement direction, float deltaTime) -> void;
        auto ProcessMouseMovement(float xpos, float ypos) -> void;

        auto setFrustum(float_t angle, float_t ratio, float_t nearD, float_t farD) -> void;
        auto setFrustumAspect(f32 ratio) -> void;
        auto setFrustumDef(glm::vec3 position, glm::vec3 target, glm::vec3 up) -> void;

        [[nodiscard]] auto inFrustum(glm::vec3 position, uint32_t scale) const -> bool;
        [[nodiscard]] auto inFrustum(glm::vec2 position, uint32_t scale) const -> bool;

        [[nodiscard]] auto getCameraPosition() const -> glm::vec3;
        [[nodiscard]] auto getCameraFront() const ->  glm::vec3;
        [[nodiscard]] auto getCameraMask() const -> u8;
        auto GetViewMatrix() const -> glm::mat4;
        auto update() -> void override;

    private:

        // --------------------------------
        // camera 3d world space attributes

        glm::vec3 _position;
        glm::vec3 _front;
        glm::vec3 _up;
        glm::vec3 _right;
        glm::vec3 _worldUp;

        // ------------
        // euler angles

        f32 _yaw;
        f32 _pitch;

        // ----------------------------------------------------
        // last x and y coordinate of the mouse on the 2d plane

        f32 _lastX;
        f32 _lastY;

        // --------------
        // camera options

        f32 _movementSpeed;
        f32 _mouseSensitivity;

        // ------------
        // view frustum

        culling::Frustum _frustum;
        u8               _mask;
    };
}


#endif //OPENGL_3D_ENGINE_CAMERA_H
