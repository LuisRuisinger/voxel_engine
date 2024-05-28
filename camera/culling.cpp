//
// Created by Luis Ruisinger on 17.03.24.
//

#include "culling.h"

namespace core::camera::culling {
    auto Frustum::setCamInternals(f32 fovDeg, f32 aspectRatio, f32 nClipDistance, f32 fClipDistance) -> void {
        _ratio = aspectRatio;
        _farD  = fClipDistance;
        _nearD = nClipDistance;
        _angle = DEG2RAD * fovDeg * 0.5f;
        _tang  = tan(_angle);

        auto angleX = atan(_tang * _ratio);

        _sphereFactorX = 1.0f / cos(angleX);
        _sphereFactorY = 1.0f / cos(_angle);
    }

    auto Frustum::setCamDef(glm::vec3 position, glm::vec3 target, glm::vec3 up) -> void {
        _camPos = position;

        _zVec = glm::normalize(target - position);
        _xVec = glm::cross(_zVec, up);
        _xVec = glm::normalize(_xVec);
        _yVec = glm::cross(_xVec, _zVec);
    }

    auto Frustum::cubeVisible(const glm::vec3 &point, u32 scale) const -> bool {
        auto radius = static_cast<f32>(scale) * sqrt(2.0F);
        return sphereInFrustum(point, radius) != OUTSIDE;
    }

    auto Frustum::squareVisible(const glm::vec2 &point, u32 scale) const -> bool {
        auto radius = static_cast<f32>(scale) * sqrt(2.0F);
        return circleInFrustum(point, radius) != OUTSIDE;
    }

    auto Frustum::cube_visible_type(const glm::vec3 &point, u32 scale) const -> CollisionType {
        auto radius = static_cast<f32>(scale) * sqrt(2.0F);
        return sphereInFrustum(point, radius);
    }

    auto Frustum::squere_visible_type(const glm::vec2 &point, u32 scale) const -> CollisionType {
        auto radius = static_cast<f32>(scale) * sqrt(2.0F);
        return circleInFrustum(point, radius);
    }

    auto Frustum::sphereInFrustum(const glm::vec3 &center, f32 radius) const -> CollisionType {
        glm::vec3 v = center - _camPos;

        auto ax = glm::dot(v, _xVec);
        auto ay = glm::dot(v, _yVec);
        auto az = glm::dot(v, _zVec);

        auto azT = az * _tang;
        auto sr  = _sphereFactorX * radius;

        if (ay > azT + sr || ay < -azT - sr)
            return OUTSIDE;

        auto maxAzX = azT * _ratio;
        if (ax > maxAzX + sr || ax < -maxAzX - sr)
            return OUTSIDE;

        if (ay > azT - sr || ay < -azT + sr)
            return INTERSECT;

        if (ax > maxAzX - sr || -maxAzX + sr)
            return INTERSECT;

        return INSIDE;
    }

    auto Frustum::circleInFrustum(const glm::vec2 &center, f32 radius) const -> CollisionType {
        glm::vec2 v = center - glm::vec2{_camPos.x, _camPos.z};
        auto az = glm::dot(v, glm::vec2{_zVec.x, _zVec.z});

        return (az > _farD + radius || az < _nearD - radius) ? OUTSIDE : INTERSECT;
    }
}
