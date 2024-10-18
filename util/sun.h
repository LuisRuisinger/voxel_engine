//
// Created by Luis Ruisinger on 05.09.24.
//

#ifndef OPENGL_3D_ENGINE_SUN_H
#define OPENGL_3D_ENGINE_SUN_H

#include "aliases.h"
#include "traits.h"
#include "camera.h"
#include "aabb.h"

#include <vector>

namespace util::sun {
    constexpr const f32 shadow_map_resolution = 1024.0F;

    class Frustum {
    public:
        Frustum();

        auto transform(const glm::mat4 &) -> void;
        auto calc_corners(const glm::mat4 &, const glm::mat4 &) -> void;
        auto calc_center() -> glm::vec3;
        auto calc_aabb() -> void;
        auto calc_ortho_proj() -> glm::mat4;

    private:
        std::vector<glm::vec4> corners;
        aabb::AABB<f32> aabb;
    };

    class Sun : public traits::Tickable<Sun> {
    public:
        Sun();

        auto tick(core::state::State &) -> void;
        auto get_orientation() -> const glm::vec3 &;

        std::vector<glm::mat4> light_space_matrices;
        std::vector<f32> shadow_cascades_level;

    private:
        auto calc_light_space_matrices(core::state::State &state) -> void;
        auto calc_light_space_matrix_level(core::state::State &, const f32, const f32) -> glm::mat4;

        glm::vec3 orientation = { 0.0F, 0.0F, 1.0F };
    };
}

#endif //OPENGL_3D_ENGINE_SUN_H
