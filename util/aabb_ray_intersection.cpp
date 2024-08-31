//
// Created by Luis Ruisinger on 28.08.24.
//

#include "aabb_ray_intersection.h"
#include "aabb.h"

#define MAX_INTERACTION_RANGE 6

namespace util::aabb_ray_intersection {
    Ray::Ray(const glm::vec3 &origin, const glm::vec3 &direction)
        : origin    { origin    },
          direction { direction }
    {
        this->direction = glm::normalize(this->direction);
    }

    auto Ray::intersect(core::level::platform::Platform &platform) -> Intersection {
        auto opt = platform.get_nearest_chunks(this->origin);
        if (!opt)
            return std::nullopt;

        const auto &world_root = platform.get_world_root();
        this->origin.x -= world_root.x;
        this->origin.z -= world_root.y;

        static std::function<f32(const glm::vec3 &, const u32)> fun =
                [this](const glm::vec3 &pos, const u32 scale) -> f32 {
                        if (scale == BASE_SIZE) {
                            return aabb::AABB<f32>()
                                    .translate(pos)
                                    .translate(0.5F)
                                    .intersection(this->origin, this->direction);
                        }
                        else {
                            return aabb::AABB<f32>()
                                    .translate(pos)
                                    .scale_center(static_cast<f32>(scale))
                                    .scale_center(0.5F)
                                    .intersection(this->origin, this->direction);
                        }
        };

        auto ray_scale = std::numeric_limits<f32>::max();
        for (const auto& chunk : opt.value()) {
            if (!chunk)
                continue;

            auto ret = chunk->find(fun);
            ray_scale = ret < ray_scale ? ret : ray_scale;
        }

        if (ray_scale > 6)
            return std::nullopt;

        auto pair = std::make_pair(this->origin, ray_scale * this->direction);
        return std::make_optional(std::move(pair));
    }
}