//
// Created by Luis Ruisinger on 17.03.24.
//

#ifndef OPENGL_3D_ENGINE_CULLING_H
#define OPENGL_3D_ENGINE_CULLING_H

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../global.h"

#define DEG2RAD 0.017453292F

namespace Culling {
    enum CollisionType {
        OUTSIDE, INTERSECT
    };

    class Frustum {
    private:
        vec3f camPos;
        vec3f x;
        vec3f y;
        vec3f z;

        f32   farD;
        f32   nearD;
        f32   ratio;
        f32   tang;

        f32   sphereFactorX;
        f32   sphereFactorY;
        f32   angle;

        [[nodiscard]] auto sphereInFrustum(const vec3f &point, f32 radius) const -> CollisionType;
        [[nodiscard]] auto circleInFrustum(const vec2f &point, f32 radius) const -> CollisionType;

    public:
        Frustum()  = default;
        ~Frustum() = default;

        auto setCamInternals(f32 angle, f32 ratio, f32 nearD, f32 farD) -> void;
        auto setCamDef(vec3f position, vec3f target, vec3f up) -> void;

        [[nodiscard]] auto cubeVisible(const vec3f &point, u32 scale) const -> bool;
        [[nodiscard]] auto squareVisible(const vec2f &point, u32 scale) const -> bool;
    };
};


#endif //OPENGL_3D_ENGINE_CULLING_H
