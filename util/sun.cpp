//
// Created by Luis Ruisinger on 05.09.24.
//

#include "sun.h"
#include "log.h"
#include "player.h"

namespace util::sun {
    constexpr const f32 sun_height = 256.0F;
    constexpr const glm::vec3 axis = { 1.0F, 0.0F, 0.0F };
    constexpr const glm::vec3 sun_pos = sun_height * axis;
    constexpr const glm::vec3 sun_up = { 0.0F, 1.0F, 0.0F };

    Sun::Sun()
        : sun_view { sun_pos, sun_up, -90.0F, PITCH }
    {
        this->sun_projection_matrix = glm::ortho(
                -384.0F, 384.0F,
                -384.0F, 384.0F,
                1.0F, 384.0F);
    }

    auto Sun::update(core::state::State &state) -> void {

    }

    auto Sun::tick(core::state::State &state) -> void {
        this->current_tick_count = (this->current_tick_count + 1) % this->max_tick_count;
        this->rotation_mat = glm::rotate(
                glm::mat4(1),
                this->rotation_angle_radians,
                axis);
        this->orientation = glm::normalize(
                glm::vec3(this->rotation_mat * glm::vec4(this->orientation, 1.0F)));

        // update sun view as object of the world
        const f32 pitch_increase = 360.0F / static_cast<f32>(this->max_tick_count);

        glm::vec3 offset = state.player.get_camera().get_position();

        const f32 texel_size = 1.0F / (CHUNK_SIZE * 2 * RENDER_RADIUS);
        glm::vec3 cam_pos = this->orientation * sun_height + offset;
        glm::vec3 snapped_camera = glm::floor(cam_pos / texel_size) * texel_size;

        this->sun_view.increase_pitch(-pitch_increase);
        this->sun_view.set_position(snapped_camera);
        this->sun_view.update();
    }

    auto Sun::get_orientation() -> const glm::vec3 & {
        return this->orientation;
    }

    auto Sun::get_view_matrix() -> const glm::mat4 & {
        return this->sun_view.get_view_matrix();
    }

    auto Sun::get_projection_matrix() -> const glm::mat4 & {
        return this->sun_projection_matrix;
    }
}