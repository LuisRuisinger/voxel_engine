//
// Created by Luis Ruisinger on 17.03.24.
//

#ifndef OPENGL_3D_ENGINE_CULLING_H
#define OPENGL_3D_ENGINE_CULLING_H

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "aliases.h"

#define DEG2RAD 0.017453292F

namespace util::culling {
    enum CollisionType : u8 {
        OUTSIDE,
        INTERSECT,
        INSIDE
    };

    class Frustum {
    public:
        Frustum() =default;
        ~Frustum() =default;

        auto set_cam_internals(f32 angle, f32 ratio, f32 nearD, f32 farD) -> void;
        auto set_cam_definition(glm::vec3 position, glm::vec3 target, glm::vec3 up) -> void;

        auto cube_visible(const glm::vec3 &point, u32 scale) const -> bool;
        auto square_visible(const glm::vec2 &point, u32 scale) const -> bool;

        auto cube_visible(glm::vec3 &&point, u32 scale) const -> bool;
        auto square_visible(glm::vec2 &&point, u32 scale) const -> bool;

        auto cube_visible_type(const glm::vec3 &point, u32 scale) const -> CollisionType;
        auto squere_visible_type(const glm::vec2 &point, u32 scale) const -> CollisionType;

        auto cube_visible_type(glm::vec3 &&point, u32 scale) const -> CollisionType;
        auto squere_visible_type(glm::vec2 &&point, u32 scale) const -> CollisionType;

    private:
        auto sphere_frustum_collision(const glm::vec3 &point, f32 radius) const -> CollisionType;
        auto cirle_frustuM_collision(const glm::vec2 &point, f32 radius) const -> CollisionType;

        auto sphere_frustum_collision(glm::vec3 &&point, f32 radius) const -> CollisionType;
        auto cirle_frustuM_collision(glm::vec2 &&point, f32 radius) const -> CollisionType;

        glm::vec3 cam_pos;
        glm::vec3 x_vec;
        glm::vec3 y_vec;
        glm::vec3 z_vec;

        f32 far_distance;
        f32 near_distance;
        f32 ratio;
        f32 tang;

        f32 sphere_factor_x;
        f32 sphere_factor_y;
        f32 angle;
    };
};


#endif //OPENGL_3D_ENGINE_CULLING_H
