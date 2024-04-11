//
// Created by Luis Ruisinger on 17.03.24.
//

#include <immintrin.h>
#include "Culling.h"

namespace Culling {
    auto Frustum::setCamInternals(f32 fovDeg, f32 aspectRatio, f32 nClipDistance, f32 fClipDistance) -> void {
        this->ratio = aspectRatio;
        this->farD  = fClipDistance;
        this->nearD = nClipDistance;
        this->angle = DEG2RAD * fovDeg * 0.5f;
        this->tang  = tan(this->angle);

        auto angleX = atan(this->tang * this->ratio);
        this->sphereFactorX = 1.0f / cos(angleX);
        this->sphereFactorY = 1.0f / cos(this->angle);
    }

    auto Frustum::setCamDef(vec3f position, vec3f target, vec3f up) -> void {
        this->camPos = position;

        this->z = glm::normalize(target - position);
        this->x = glm::cross(this->z, up);
        this->x = glm::normalize(this->x);
        this->y = glm::cross(this->x, this->z);
    }

    auto Frustum::cubeVisible(const vec3f &point, u32 scale) const -> bool {
        f32 radius = (f32) (scale * sqrt(2.0));
        return sphereInFrustum(point, radius) != OUTSIDE;
    }

    auto Frustum::squareVisible(const vec2f &point, u32 scale) const -> bool {
        f32 radius = (f32) (scale * sqrt(2.0));
        return circleInFrustum(point, radius) != OUTSIDE;
    }

    auto Frustum::sphereInFrustum(const vec3f &center, f32 radius) const -> CollisionType {
        vec3f v = center - this->camPos;

        auto ax = glm::dot(v, this->x);
        auto ay = glm::dot(v, this->y);
        auto az = glm::dot(v, this->z);

        auto azT = az * this->tang;
        auto sr  = this->sphereFactorX * radius;

        auto maxAzX = azT * this->ratio + sr;
        auto maxAzY = azT + sr;

        return (ax > maxAzX || ax < -maxAzX || ay > maxAzY || ay < -maxAzY) ? OUTSIDE : INTERSECT;
    }

    auto Frustum::circleInFrustum(const vec2f &center, f32 radius) const -> CollisionType {
        vec2f v = center - glm::vec2{this->camPos.x, this->camPos.z};
        auto az = glm::dot(v, glm::vec2{this->z.x, this->z.z});

        return (az > this->farD + radius || az < this->nearD - radius) ? OUTSIDE : INTERSECT;
    }
}
