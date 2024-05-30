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
        setFrustum(45.0f, 1800.0 / 1200.0, -2.5F, CHUNK_SIZE * (RENDER_RADIUS * 2));
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
        setFrustum(45.0f, 1800.0 / 1200.0, -2.5F, CHUNK_SIZE * (RENDER_RADIUS * 2));
        update();
    }

    void Camera::ProcessKeyboard(Camera_Movement direction, f32 deltaTime) {
        f32 velocity = _movementSpeed * deltaTime;

        switch (direction) {
            case FORWARD : _position += _front * velocity; break;
            case BACKWARD: _position -= _front * velocity; break;
            case LEFT    : _position -= _right * velocity; break;
            case RIGHT   : _position += _right * velocity; break;
            case UP      : _position += _up * velocity;    break;
            case DOWN    : _position -= _up * velocity;    break;
        }

        update();
    }

    auto Camera::ProcessMouseMovement(f32 xpos, f32 ypos) -> void {
        f32 xOffset = xpos - _lastX;
        f32 yOffset = _lastY - ypos;

        _lastX = xpos;
        _lastY = ypos;

        _yaw   += xOffset * _mouseSensitivity;
        _pitch += yOffset * _mouseSensitivity;

        if (_pitch > 89.0f)
            _pitch = 89.0f;
        if (_pitch < -89.0f)
            _pitch = -89.0f;

        update();
    }

    auto Camera::update() -> void {
        f32 cosYaw   = cos(glm::radians(_yaw));
        f32 cosPitch = cos(glm::radians(_pitch));

        _front = {
                cosYaw * cosPitch,
                sin(glm::radians(_pitch)),
                sin(glm::radians(_yaw)) * cosPitch
        };

        _front = glm::normalize(_front);
        _right = glm::normalize(glm::cross(_front, _worldUp));
        _up    = glm::normalize(glm::cross(_right, _front));

        setFrustumDef(_position, _front + _position, _up);

        _mask  = UINT8_MAX;
        _mask ^= (_front.x >  0.55) * (RIGHT_BIT  >> 10);
        _mask ^= (_front.x < -0.55) * (LEFT_BIT   >> 10);
        _mask ^= (_front.y >  0.25) * (TOP_BIT    >> 10);
        _mask ^= (_front.y < -0.25) * (BOTTOM_BIT >> 10);
        _mask ^= (_front.z >  0.55) * (FRONT_BIT  >> 10);
        _mask ^= (_front.z < -0.55) * (BACK_BIT   >> 10);
    }

    auto Camera::setFrustum(f32 angle, f32 ratio, f32 nearD, f32 farD) -> void {
        _frustum.setCamInternals(angle, ratio, nearD, farD);
    }

    auto Camera::setFrustumAspect(f32 ratio) -> void {
        _frustum.setCamInternals(45.0f, ratio, -5.0f, CHUNK_SIZE * (RENDER_RADIUS * 2));
    }

    auto Camera::setFrustumDef(glm::vec3 pos, glm::vec3 target, glm::vec3 varUp) -> void {
        _frustum.setCamDef(pos, target, varUp);
    }

    auto Camera::inFrustum(glm::vec2 pos, u32 scale) const -> bool {
        return _frustum.squareVisible(pos, scale);
    }

    auto Camera::inFrustum(glm::vec3 pos, u32 scale) const -> bool {
        return _frustum.cubeVisible(pos, scale);
    }

    auto Camera::inFrustum_type(glm::vec2 pos, u32 scale) const -> camera::culling::CollisionType {
        return _frustum.squere_visible_type(pos, scale);
    }
    auto Camera::inFrustum_type(glm::vec3 pos, u32 scale) const -> camera::culling::CollisionType {
        return _frustum.cube_visible_type(pos, scale);
    }

    auto Camera::getCameraPosition() const -> glm::vec3 {
        return _position;
    }

    auto Camera::getCameraFront() const -> glm::vec3 {
        return _front;
    }

    auto Camera::getCameraMask() const -> u8 {
        return _mask;
    }

    auto Camera::GetViewMatrix() const -> glm::mat4 {
        return glm::lookAt(_position, _position + _front, _up);
    }
}
