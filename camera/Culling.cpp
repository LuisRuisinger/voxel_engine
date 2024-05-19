//
// Created by Luis Ruisinger on 17.03.24.
//

#include <immintrin.h>
#include "Culling.h"

namespace Culling {
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

    auto Frustum::setCamDef(vec3f position, vec3f target, vec3f up) -> void {
        _camPos = position;

        _zVec = glm::normalize(target - position);
        _xVec = glm::cross(_zVec, up);
        _xVec = glm::normalize(_xVec);
        _yVec = glm::cross(_xVec, _zVec);
    }

    auto Frustum::cubeVisible(const vec3f &point, u32 scale) const -> bool {
        auto radius = static_cast<f32>(scale * sqrt(2.0));
        return sphereInFrustum(point, radius) != OUTSIDE;
    }

    auto Frustum::squareVisible(const vec2f &point, u32 scale) const -> bool {
        auto radius = static_cast<f32>(scale * sqrt(2.0));
        return circleInFrustum(point, radius) != OUTSIDE;
    }

    auto Frustum::sphereInFrustum(const vec3f &center, f32 radius) const -> CollisionType {
        vec3f v = center - _camPos;

        auto ax = glm::dot(v, _xVec);
        auto ay = glm::dot(v, _yVec);
        auto az = glm::dot(v, _zVec);

        auto azT = az * _tang;
        auto sr  = _sphereFactorX * radius;

        auto maxAzX = azT * _ratio + sr;
        auto maxAzY = azT + sr;

        return (ax > maxAzX || ax < -maxAzX || ay > maxAzY || ay < -maxAzY) ? OUTSIDE : INTERSECT;
    }

    auto Frustum::circleInFrustum(const vec2f &center, f32 radius) const -> CollisionType {
        vec2f v = center - glm::vec2{_camPos.x, _camPos.z};
        auto az = glm::dot(v, glm::vec2{_zVec.x, _zVec.z});

        return (az > _farD + radius || az < _nearD - radius) ? OUTSIDE : INTERSECT;
    }
}
