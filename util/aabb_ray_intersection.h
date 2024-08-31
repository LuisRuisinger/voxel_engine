//
// Created by Luis Ruisinger on 28.08.24.
//

#ifndef OPENGL_3D_ENGINE_AABB_RAY_INTERSECTION_H
#define OPENGL_3D_ENGINE_AABB_RAY_INTERSECTION_H

#include "aliases.h"

#include "../core/level/platform.h"

namespace util::aabb_ray_intersection {
    using Intersection = std::optional<std::pair<glm::vec3, glm::vec3>>;

    struct Ray {
        Ray(const glm::vec3 &, const glm::vec3 &);

        // if intersection succeeds with a node
        // we return the 1^3 position of hit node
        // must keep in mind voxels bigger than base size
        auto intersect(core::level::platform::Platform &) -> Intersection;

        glm::vec3 origin;
        glm::vec3 direction;
    };
}


#endif //OPENGL_3D_ENGINE_AABB_RAY_INTERSECTION_H
