//
// Created by Luis Ruisinger on 17.03.24.
//

#ifndef OPENGL_3D_ENGINE_CULLING_H
#define OPENGL_3D_ENGINE_CULLING_H

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "../util/aliases.h"

#define DEG2RAD 0.017453292F

namespace Culling {
    enum CollisionType {
        OUTSIDE, INTERSECT
    };

    class Frustum {
    public:
        Frustum()  = default;
        ~Frustum() = default;

        auto setCamInternals(f32 angle, f32 ratio, f32 nearD, f32 farD) -> void;
        auto setCamDef(vec3f position, vec3f target, vec3f up) -> void;

        [[nodiscard]] auto cubeVisible(const vec3f &point, u32 scale) const -> bool;
        [[nodiscard]] auto squareVisible(const vec2f &point, u32 scale) const -> bool;

    private:
        vec3f _camPos;
        vec3f _xVec;
        vec3f _yVec;
        vec3f _zVec;

        f32   _farD;
        f32   _nearD;
        f32   _ratio;
        f32   _tang;

        f32   _sphereFactorX;
        f32   _sphereFactorY;
        f32   _angle;

        [[nodiscard]] auto sphereInFrustum(const vec3f &point, f32 radius) const -> CollisionType;
        [[nodiscard]] auto circleInFrustum(const vec2f &point, f32 radius) const -> CollisionType;
    };
};


#endif //OPENGL_3D_ENGINE_CULLING_H
