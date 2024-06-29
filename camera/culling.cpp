//
// Created by Luis Ruisinger on 17.03.24.
//

#include "culling.h"

namespace core::camera::culling {
    auto Frustum::setCamInternals(f32 fovDeg, f32 aspectRatio, f32 nClipDistance, f32 fClipDistance) -> void {
        this->_ratio = aspectRatio;
        this->_farD  = fClipDistance;
        this->_nearD = nClipDistance;
        this->_angle = DEG2RAD * fovDeg * 0.5f;
        this->_tang  = tan(this->_angle);

        auto angleX = atan(this->_tang * this->_ratio);

        this->_sphereFactorX = 1.0f / cos(angleX);
        this->_sphereFactorY = 1.0f / cos(this->_angle);
    }

    auto Frustum::setCamDef(glm::vec3 position, glm::vec3 target, glm::vec3 up) -> void {
        this->_camPos = position;

        this->_zVec = glm::normalize(target - position);
        this->_xVec = glm::cross(this->_zVec, up);
        this->_xVec = glm::normalize(this->_xVec);
        this->_yVec = glm::cross(this->_xVec, this->_zVec);
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
        glm::vec3 v = center - this->_camPos;

        auto ax = glm::dot(v, this->_xVec);
        auto ay = glm::dot(v, this->_yVec);
        auto az = glm::dot(v, this->_zVec);

        auto azT = az * this->_tang;
        auto sr  = this->_sphereFactorX * radius;

        if (ay > azT + sr || ay < -azT - sr)
            return OUTSIDE;

        auto maxAzX = azT * this->_ratio;
        if (ax > maxAzX + sr || ax < -maxAzX - sr)
            return OUTSIDE;

        if (ay > azT - sr || ay < -azT + sr)
            return INTERSECT;

        if (ax > maxAzX - sr || -maxAzX + sr)
            return INTERSECT;

        return INSIDE;
    }

    auto Frustum::circleInFrustum(const glm::vec2 &center, f32 radius) const -> CollisionType {
        glm::vec2 v = center - glm::vec2 {this->_camPos.x, this->_camPos.z};
        auto az = glm::dot(v, glm::vec2 {this->_zVec.x, this->_zVec.z});

        return (az > this->_farD + radius || az < this->_nearD - radius) ? OUTSIDE : INTERSECT;
    }
}
