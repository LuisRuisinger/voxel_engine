//
// Created by Luis Ruisinger on 15.02.24.
//

#include "camera.h"
#include "Level/Platform.h"

namespace Camera {
    Camera::Camera(vec3f initPosition, vec3f initUp, f32 initYaw, f32 initPitch)
        : position{initPosition}
        , front{}
        , up{}
        , right{}
        , worldUp{initUp}
        , yaw{initYaw}
        , pitch{initPitch}
        , movementSpeed{SPEED}
        , mouseSensitivity{SENSITIVITY}
        , frustum{}
        , mask{UINT8_MAX} {

        setFrustum(45.0f, 1800.0 / 1200.0, -5.0f, CHUNK_SIZE * (RENDER_RADIUS * 2));
        updateCameraVectors();
    }

    Camera::Camera(f32 posX, f32 posY, f32 posZ, f32 upX, f32 upY, f32 upZ, f32 yaw, f32 pitch)
        : position(posX, posY, posZ)
        , front{}
        , up{}
        , right{}
        , worldUp(upX, upY, upZ)
        , yaw{yaw}
        , pitch{pitch}
        , movementSpeed{SPEED}
        , mouseSensitivity{SENSITIVITY}
        , frustum{}
        , mask{UINT8_MAX} {

        setFrustum(45.0f, 1800.0 / 1200.0, -5.0f, CHUNK_SIZE * (RENDER_RADIUS * 2));
        updateCameraVectors();
    }

    auto Camera::GetViewMatrix() -> glm::mat4 {
        return glm::lookAt(this->position, this->position + this->front, this->up);
    }

    void Camera::ProcessKeyboard(Camera_Movement direction, f32 deltaTime) {
        f32 velocity = this->movementSpeed * deltaTime;

        switch (direction) {
            case FORWARD : this->position += this->front * velocity; break;
            case BACKWARD: this->position -= this->front * velocity; break;
            case LEFT    : this->position -= this->right * velocity; break;
            case RIGHT   : this->position += this->right * velocity; break;
            case UP      : this->position += this->up * velocity;    break;
            case DOWN    : this->position -= this->up * velocity;    break;
        }

        updateCameraVectors();
    }

    auto Camera::ProcessMouseMovement(f32 xoffset, f32 yoffset, GLboolean constrainPitch = true) -> void {
        this->yaw   += xoffset * this->mouseSensitivity;
        this->pitch += yoffset * this->mouseSensitivity;

        if (this->pitch > 89.0f)
            this->pitch = 89.0f;
        if (this->pitch < -89.0f)
            this->pitch = -89.0f;

        updateCameraVectors();
    }

    auto Camera::updateCameraVectors() -> void {
        f32 cosYaw   = cos(glm::radians(this->yaw));
        f32 cosPitch = cos(glm::radians(this->pitch));

        this->front = {
                cosYaw * cosPitch,
                sin(glm::radians(this->pitch)),
                sin(glm::radians(this->yaw)) * cosPitch
        };

        this->front = glm::normalize(this->front);
        this->right = glm::normalize(glm::cross(this->front, this->worldUp));
        this->up    = glm::normalize(glm::cross(this->right, this->front));

        setFrustumDef(this->position, this->front + this->position, this->up);

        this->mask  = UINT8_MAX;
        this->mask ^= (this->front.x >  0.55) * (RIGHT_BIT  >> 10);
        this->mask ^= (this->front.x < -0.55) * (LEFT_BIT   >> 10);
        this->mask ^= (this->front.y >  0.25) * (TOP_BIT    >> 10);
        this->mask ^= (this->front.y < -0.25) * (BOTTOM_BIT >> 10);
        this->mask ^= (this->front.z >  0.55) * (FRONT_BIT  >> 10);
        this->mask ^= (this->front.z < -0.55) * (BACK_BIT   >> 10);
    }

    auto Camera::setFrustum(f32 angle, f32 ratio, f32 nearD, f32 farD) -> void {
        this->frustum.setCamInternals(angle, ratio, nearD, farD);
    }

    auto Camera::setFrustumAspect(f32 ratio) -> void {
        this->frustum.setCamInternals(45.0f, ratio, -5.0f, CHUNK_SIZE * (RENDER_RADIUS * 2));
    }

    auto Camera::setFrustumDef(vec3f pos, vec3f target, vec3f varUp) -> void {
        this->frustum.setCamDef(pos, target, varUp);
    }

    auto Camera::inFrustum(vec2f pos, u32 scale) const -> bool {
        return this->frustum.squareVisible(pos, scale);
    }

    auto Camera::inFrustum(vec3f pos, u32 scale) const -> bool {
        return this->frustum.cubeVisible(pos, scale);
    }

    auto Camera::getCameraPosition() const -> vec3f {
        return this->position;
    }

    auto Camera::getCameraFront() const -> vec3f {
        return this->front;
    }

    auto Camera::getCameraMask() const -> u8 {
        return this->mask;
    }
}
