//
// Created by Luis Ruisinger on 05.09.24.
//

#include "sun.h"
#include "log.h"

namespace util::sun {
    constexpr const glm::vec3 axis = { 1.0F, 0.0F, 0.0F };

    auto Sun::tick(core::state::State &) -> void {
        this->tick_count = (this->tick_count + 1) % MAX_TICK_COUNT;
        this->rotation_mat = glm::rotate(
                glm::mat4(1),
                this->rotation_angle_radians,
                axis);
        this->orientation = glm::vec3(this->rotation_mat * glm::vec4(this->orientation, 1.0F));
    }

    auto Sun::get_orientation() -> const glm::vec3 & {
        return this->orientation;
    }
}