//
// Created by Luis Ruisinger on 28.08.24.
//

#ifndef OPENGL_3D_ENGINE_AABB_H
#define OPENGL_3D_ENGINE_AABB_H

#ifdef __AVX2__
#include <immintrin.h>
#define GLM_FORCE_AVX
#endif

#include "aliases.h"

namespace util::aabb {
    template <typename T>
    struct AABB {
        using V = glm::vec<3, T, glm::defaultp>;
        using M = glm::mat<4, 4, T, glm::defaultp>;
        using Ref = AABB<T> &;

        V min;
        V max;

        AABB() : min { 0 }, max { static_cast<T>(1) } {}
        AABB(T max) : min { 0 }, max { max } {}
        AABB(T min, T max) : min { min }, max { max } {}

        INLINE auto scale(const V &v) -> Ref {
            const auto &d = this->max - this->min;
            this->max = d * v;

            return *this;
        }

        INLINE auto scale(const T &t) -> Ref {
            const auto v = V { t };
            return scale(v);
        }

        INLINE auto scale_center(const V &v) -> Ref {
            const auto &c = (this->max + this->min) / static_cast<T>(2);
            const auto &d = (this->max - this->min) / static_cast<T>(2);

            this->min = c - v * d;
            this->max = c + v * d;

            return *this;
        }

        INLINE auto scale_center(const T &t) -> Ref {
            const auto v = V { t };
            return scale_center(v);
        }

        INLINE auto translate(const V &v) -> Ref {
            this->min += v;
            this->max += v;

            return *this;
        }

        INLINE auto translate(const T &t) -> Ref {
            const auto v = V { t };
            return translate(v);
        }

        auto intersection(const glm::vec3 &o, const glm::vec3 &d) const -> f32 {
            glm::vec3 tmin_vec = (this->min - o) / d;
            glm::vec3 tmax_vec = (this->max - o) / d;

            glm::vec3 tmin = glm::min(tmin_vec, tmax_vec);
            glm::vec3 tmax = glm::max(tmin_vec, tmax_vec);

            f32 t_enter = std::max(std::max(tmin.x, tmin.y), tmin.z);
            f32 t_exit  = std::min(std::min(tmax.x, tmax.y), tmax.z);

            // if tmax < 0, ray (line) is intersecting AABB
            // but whole AABB is behind the camera
            if (t_exit < 0.0F)
                return std::numeric_limits<f32>::max();

            // if tmin > tmax, ray doesn't intersect AABB
            if (t_enter > t_exit)
                return std::numeric_limits<f32>::max();

            return t_enter < 0.0F ? t_exit : t_enter;
        }

        auto intersection(const AABB &other) const -> bool {
            return (this->min.x <= other.max.x && this->max.x >= this->min.x) &&
                   (this->min.y <= other.max.y && this->max.y >= this->min.y) &&
                   (this->min.z <= other.max.z && this->max.z >= this->min.z);
        }
    };
}

#endif //OPENGL_3D_ENGINE_AABB_H
