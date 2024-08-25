//
// Created by Luis Ruisinger on 17.03.24.
//

#include "culling.h"

namespace util::culling {
    auto Frustum::set_cam_internals(f32 angle, f32 ratio, f32 nearD, f32 farD) -> void {
        this->ratio = ratio;
        this->far_distance  = farD;
        this->near_distance = nearD;
        this->angle = DEG2RAD * angle * 0.5F;
        this->tang  = tan(this->angle);

        auto angle_x = atan(this->tang * this->ratio);

        this->sphere_factor_x = 1.0F / cos(angle_x);
        this->sphere_factor_y = 1.0F / cos(this->angle);
    }

    auto Frustum::set_cam_definition(glm::vec3 position, glm::vec3 target, glm::vec3 up) -> void {
        this->cam_pos = position;

        this->z_vec = glm::normalize(target - position);
        this->x_vec = glm::cross(this->z_vec, up);
        this->x_vec = glm::normalize(this->x_vec);
        this->y_vec = glm::cross(this->x_vec, this->z_vec);
    }

    auto Frustum::cube_visible(const glm::vec3 &point, u32 scale) const -> bool {
        auto radius = static_cast<f32>(scale) * SQRT_2;
        return sphere_frustum_collision(point, radius) != OUTSIDE;
    }

    auto Frustum::square_visible(const glm::vec2 &point, u32 scale) const -> bool {
        auto radius = static_cast<f32>(scale) * SQRT_2;
        return cirle_frustuM_collision(point, radius) != OUTSIDE;
    }

    auto Frustum::cube_visible(glm::vec3 &&point, u32 scale) const -> bool {
        auto radius = static_cast<f32>(scale) * SQRT_2;
        return sphere_frustum_collision(std::forward<glm::vec3>(point), radius) != OUTSIDE;
    }

    auto Frustum::square_visible(glm::vec2 &&point, u32 scale) const -> bool {
        auto radius = static_cast<f32>(scale) * SQRT_2;
        return cirle_frustuM_collision(std::forward<glm::vec2>(point), radius) != OUTSIDE;
    }

    auto Frustum::cube_visible_type(const glm::vec3 &point, u32 scale) const -> CollisionType {
        auto radius = static_cast<f32>(scale) * SQRT_2;
        return sphere_frustum_collision(point, radius);
    }

    auto Frustum::squere_visible_type(const glm::vec2 &point, u32 scale) const -> CollisionType {
        auto radius = static_cast<f32>(scale) * SQRT_2;
        return cirle_frustuM_collision(point, radius);
    }

    auto Frustum::cube_visible_type(glm::vec3 &&point, u32 scale) const -> CollisionType {
        auto radius = static_cast<f32>(scale) * SQRT_2;
        return sphere_frustum_collision(std::forward<glm::vec3>(point), radius);
    }

    auto Frustum::squere_visible_type(glm::vec2 &&point, u32 scale) const -> CollisionType {
        auto radius = static_cast<f32>(scale) * SQRT_2;
        return cirle_frustuM_collision(std::forward<glm::vec2>(point), radius);
    }

    auto Frustum::sphere_frustum_collision(
            const glm::vec3 &point,
            f32 radius) const -> CollisionType {
        glm::vec3 v = point - this->cam_pos;

        auto ax = glm::dot(v, this->x_vec);
        auto ay = glm::dot(v, this->y_vec);
        auto az = glm::dot(v, this->z_vec);

        auto az_t = az * this->tang;
        auto sr = this->sphere_factor_x * radius;

        if (ay > az_t + sr || ay < -az_t - sr)
            return OUTSIDE;

        auto max_az_x = az_t * this->ratio;
        if (ax > max_az_x + sr || ax < -max_az_x - sr)
            return OUTSIDE;

        if (ay > az_t - sr || ay < -az_t + sr)
            return INTERSECT;

        if (ax > max_az_x - sr || -max_az_x + sr)
            return INTERSECT;

        return INSIDE;
    }

    auto Frustum::cirle_frustuM_collision(
            const glm::vec2 &point,
            f32 radius) const -> CollisionType {
        glm::vec2 v = point - glm::vec2 {this->cam_pos.x, this->cam_pos.z};
        auto az = glm::dot(
                v,
                glm::vec2 {this->z_vec.x, this->z_vec.z});

        return (az > this->far_distance + radius || az < this->near_distance - radius)
            ? OUTSIDE : INTERSECT;
    }

    auto Frustum::sphere_frustum_collision(glm::vec3 &&point, f32 radius) const -> CollisionType {
        point -= this->cam_pos;

        auto ax = glm::dot(point, this->x_vec);
        auto ay = glm::dot(point, this->y_vec);
        auto az = glm::dot(point, this->z_vec);

        auto az_t = az * this->tang;
        auto sr  = this->sphere_factor_x * radius;

        if (ay > az_t + sr || ay < -az_t - sr)
            return OUTSIDE;

        auto max_az_x = az_t * this->ratio;
        if (ax > max_az_x + sr || ax < -max_az_x - sr)
            return OUTSIDE;

        if (ay > az_t - sr || ay < -az_t + sr)
            return INTERSECT;

        if (ax > max_az_x - sr || -max_az_x + sr)
            return INTERSECT;

        return INSIDE;
    }

    auto Frustum::cirle_frustuM_collision(glm::vec2 &&point, f32 radius) const -> CollisionType {
        point -= glm::vec2 {this->cam_pos.x, this->cam_pos.z };
        auto az = glm::dot(
                point,
                glm::vec2 {this->z_vec.x, this->z_vec.z});

        return (az > this->far_distance + radius || az < this->near_distance - radius)
            ? OUTSIDE : INTERSECT;
    }
}
