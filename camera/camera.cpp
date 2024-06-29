//
// Created by Luis Ruisinger on 15.02.24.
//

#include "camera.h"
#include "../level/platform.h"

namespace core::camera::perspective {
    Camera::Camera(glm::vec3 initPosition, glm::vec3 initUp, f32 initYaw, f32 initPitch)
        : _position{initPosition}
        , _front{}
        , _up{}
        , _right{}
        , _worldUp{initUp}
        , _yaw{initYaw}
        , _pitch{initPitch}
        , _movementSpeed{SPEED}
        , _mouseSensitivity{SENSITIVITY}
        , _frustum{}
        , _mask{UINT8_MAX}
    {
        setFrustum(45.0f, 1800.0 / 1200.0, -2.0F, 32.0F * (RENDER_RADIUS * 2));
        update();
    }

    Camera::Camera(f32 posX, f32 posY, f32 posZ, f32 upX, f32 upY, f32 upZ, f32 yaw, f32 pitch)
        : _position(posX, posY, posZ)
        , _front{}
        , _up{}
        , _right{}
        , _worldUp{upX, upY, upZ}
        , _yaw{yaw}
        , _pitch{pitch}
        , _movementSpeed{SPEED}
        , _mouseSensitivity{SENSITIVITY}
        , _frustum{}
        , _mask{UINT8_MAX}
    {
        setFrustum(45.0f, 1800.0 / 1200.0, -2.0F, 32.0F * (RENDER_RADIUS * 2));
        update();
    }

    void Camera::ProcessKeyboard(Camera_Movement direction, f32 deltaTime) {
        f32 velocity = this->_movementSpeed * deltaTime;

        switch (direction) {
            case FORWARD : this->_position += this->_front * velocity; break;
            case BACKWARD: this->_position -= this->_front * velocity; break;
            case LEFT    : this->_position -= this->_right * velocity; break;
            case RIGHT   : this->_position += this->_right * velocity; break;
            case UP      : this->_position += this->_up * velocity;    break;
            case DOWN    : this->_position -= this->_up * velocity;    break;
        }

        update();
    }

    auto Camera::ProcessMouseMovement(f32 xpos, f32 ypos) -> void {
        f32 xOffset = xpos - this->_lastX;
        f32 yOffset = this->_lastY - ypos;

        this->_lastX = xpos;
        this->_lastY = ypos;

        this->_yaw   += xOffset * this->_mouseSensitivity;
        this->_pitch += yOffset * this->_mouseSensitivity;

        if (this->_pitch > 89.0f)
            this->_pitch = 89.0f;
        if (this->_pitch < -89.0f)
            this->_pitch = -89.0f;

        update();
    }

    auto Camera::update() -> void {
        f32 cosYaw   = cos(glm::radians(this->_yaw));
        f32 cosPitch = cos(glm::radians(this->_pitch));

        this->_front = {
                cosYaw * cosPitch,
                sin(glm::radians(this->_pitch)),
                sin(glm::radians(this->_yaw)) * cosPitch
        };

        this->_front = glm::normalize(this->_front);
        this->_right = glm::normalize(glm::cross(this->_front, this->_worldUp));
        this->_up    = glm::normalize(glm::cross(this->_right, this->_front));

        setFrustumDef(this->_position, this->_front + this->_position, this->_up);

        this->_mask  = UINT8_MAX;
        this->_mask ^= (this->_front.x >  0.55) * (RIGHT_BIT  >> 10);
        this->_mask ^= (this->_front.x < -0.55) * (LEFT_BIT   >> 10);
        this->_mask ^= (this->_front.y >  0.25) * (TOP_BIT    >> 10);
        this->_mask ^= (this->_front.y < -0.25) * (BOTTOM_BIT >> 10);
        this->_mask ^= (this->_front.z >  0.55) * (FRONT_BIT  >> 10);
        this->_mask ^= (this->_front.z < -0.55) * (BACK_BIT   >> 10);
    }

    auto Camera::setFrustum(f32 angle, f32 ratio, f32 nearD, f32 farD) -> void {
        this->_frustum.setCamInternals(angle, ratio, nearD, farD);
    }

    auto Camera::setFrustumAspect(f32 ratio) -> void {
        this->_frustum.setCamInternals(45.0f, ratio, -5.0f, 32.0F * (RENDER_RADIUS * 2));
    }

    auto Camera::setFrustumDef(glm::vec3 pos, glm::vec3 target, glm::vec3 varUp) -> void {
        this->_frustum.setCamDef(pos, target, varUp);
    }

    auto Camera::inFrustum(glm::vec2 pos, u32 scale) const -> bool {
        return this->_frustum.squareVisible(pos, scale);
    }

    auto Camera::inFrustum(glm::vec3 pos, u32 scale) const -> bool {
        return this->_frustum.cubeVisible(pos, scale);
    }

    auto Camera::inFrustum_type(glm::vec2 pos, u32 scale) const -> camera::culling::CollisionType {
        return this->_frustum.squere_visible_type(pos, scale);
    }
    auto Camera::inFrustum_type(glm::vec3 pos, u32 scale) const -> camera::culling::CollisionType {
        return this->_frustum.cube_visible_type(pos, scale);
    }

    auto Camera::getCameraPosition() const -> glm::vec3 {
        return this->_position;
    }

    auto Camera::getCameraFront() const -> glm::vec3 {
        return this->_front;
    }

    auto Camera::getCameraMask() const -> u8 {
        return this->_mask;
    }

    auto Camera::GetViewMatrix() const -> glm::mat4 {
        return glm::lookAt(this->_position, this->_position + this->_front, this->_up);
    }
}
