//
// Created by Luis Ruisinger on 05.09.24.
//

#ifndef OPENGL_3D_ENGINE_SUN_H
#define OPENGL_3D_ENGINE_SUN_H

#include "aliases.h"
#include "traits.h"

namespace util::sun {
    class Sun :
            public traits::Tickable<Sun>,
            public traits::Updateable<Sun> {
    public:

        // TODO: update sun matrices here
        auto tick(core::state::State &) -> void;

        // update can actually be left empty afaik
        // because the angle of orientation changes per tick and not per frame
        // thus matrices can be calculated in the tick and not update

        // keep for completeness of logic
        // call will be removed by the compiler
        // not if put in a threadpool afaik
        auto update(core::state::State &) -> void;
        auto get_orientation() -> const glm::vec3 &;

    private:

        // offset for sun view
        // used for shadow map calculations through the sun
        const f32 height = 512.0F;
        f32 sun_scalar_offset = RENDER_RADIUS * CHUNK_SIZE * 2.0F;

        u32 max_tick_count = 24000;
        u32 current_tick_count = 0;

        // used for sun rotation
        f32 rotation_angle_radians = -2.0F * M_PI / static_cast<f32>(this->max_tick_count);
        glm::mat4 rotation_mat;
        glm::vec3 orientation = { 0.0F, 0.0F, 1.0F };

        // TODO: add orthogonal perspective camera for sun shadows
    };
}

#endif //OPENGL_3D_ENGINE_SUN_H
