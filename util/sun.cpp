//
// Created by Luis Ruisinger on 05.09.24.
//

#include "sun.h"
#include "log.h"
#include "player.h"

namespace util::sun {
    constexpr const f32 max_degrees = 2.0 * M_PI;
    constexpr const glm::vec3 axis = { 1.0F, 0.0F, 0.0F };
    constexpr const glm::vec3 sun_angle = { 0.0F, 0.0F, 1.0F };

    Sun::Sun() : shadow_cascades_level { 16.0F, 48.0F, 128.0F, 384.0F } {}

    auto Sun::tick(core::state::State &state) -> void {
        const f32 rotation_angle = -2.0F * M_PI / static_cast<f32>(state.max_tick_count);

        auto trans = glm::mat4(1);
        trans = glm::rotate(trans, rotation_angle, axis);

        this->orientation = glm::normalize(glm::vec3(trans * glm::vec4(this->orientation, 1.0F)));
        calc_light_space_matrices(state);
    }

    auto Sun::calc_light_space_matrices(core::state::State &state) -> void {
        std::vector<decltype(this->light_space_matrices)::value_type> swap;

        auto near_plane = camera::near_plane;
        for (auto &far_plane : this->shadow_cascades_level) {
            swap.push_back(calc_light_space_matrix_level(state, near_plane, far_plane));
            near_plane = far_plane;
        }

        this->light_space_matrices = std::move(swap);
    }

    auto Sun::calc_light_space_matrix_level(
            core::state::State &state, const f32 near, const f32 far) -> glm::mat4 {
        constexpr const auto up = glm::vec3(0.0F, 1.0F, 0.0F);

        const auto &player_view = state.player.get_camera().get_view_matrix();
        const auto &player_proj = state.player.get_camera().get_projection_matrix();

        // used align the shadow map to the texel per shadow map
        const f32 texels_per_unit = (far - near) / shadow_map_resolution;

        // TODO: maybe move this in a seperate struct
        const f32 aspect_ratio = player_proj[1][1] / player_proj[0][0];
        const f32 fov = 2.0F * atan(1.0F / player_proj[1][1]);
        const auto proj = glm::perspective(fov, aspect_ratio, near, far);

        auto frustum = Frustum {};
        frustum.calc_corners(proj, player_view);

        auto center = frustum.calc_center();
        center = glm::floor(center / texels_per_unit) * texels_per_unit;

        const auto light_view = glm::lookAt(center + this->orientation, center, up);
        frustum.transform(light_view);
        frustum.calc_aabb();

        const auto ortho = frustum.calc_ortho_proj();
        return ortho * light_view;
    }

    auto Sun::get_orientation() -> const glm::vec3 & {
        return this->orientation;
    }

    Frustum::Frustum()
        : aabb { std::numeric_limits<f32>::max(), std::numeric_limits<f32>::lowest() }
    {}

    auto Frustum::calc_corners(const glm::mat4 &proj, const glm::mat4 &view) -> void {
        const auto inv_view_proj = glm::inverse(proj * view);

        for (auto x = 0; x < 2; ++x) {
            for (auto y = 0; y < 2; ++y) {
                for (auto z = 0; z < 2; ++z) {
                    auto corner = inv_view_proj * glm::vec4 {
                            2.0F * x - 1.0F,
                            2.0F * y - 1.0F,
                            2.0F * z - 1.0F,
                            1.0F
                    };

                    corner /= corner.w;
                    this->corners.push_back(std::move(corner));
                }
            }
        }
    }

    auto Frustum::calc_center() -> glm::vec3 {
        auto center = glm::vec3(0.0F);

        for (const auto &v : this->corners)
            center += glm::vec3(v);

        center /= this->corners.size();
        return center;
    }

    auto Frustum::calc_aabb() -> void {

        // scales the shadow frustum in all directions
        // constexpr const auto shadow_map_scaling = glm::vec3(1.5F, 1.5F, 1.0F);

        // scales the shadow frustum the z coordinate
        constexpr const f32 shadow_map_z_scaling = 16.0F;
        constexpr const f32 shadow_map_inv_z_scaling = 1.0F / shadow_map_z_scaling;

        for (const auto &v : this->corners) {
            this->aabb.add(v);
        }

        // this->aabb.scale_center(shadow_map_scaling);
        this->aabb.min.z *= (this->aabb.min.z < 0.0F) ? shadow_map_z_scaling : shadow_map_inv_z_scaling;
        this->aabb.max.z *= (this->aabb.max.z < 0.0F) ? shadow_map_inv_z_scaling : shadow_map_z_scaling;
    }

    auto Frustum::calc_ortho_proj() -> glm::mat4 {
        return glm::ortho(
                this->aabb.min.x, this->aabb.max.x,
                this->aabb.min.y, this->aabb.max.y,
                this->aabb.min.z, this->aabb.max.z);
    }

    auto Frustum::transform(const glm::mat4 &mat) -> void {
        for (auto &v : this->corners) {
            v = mat * v;
        }
    }
}