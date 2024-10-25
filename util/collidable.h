//
// Created by Luis Ruisinger on 02.09.24.
//

#ifndef OPENGL_3D_ENGINE_COLLIDABLE_H
#define OPENGL_3D_ENGINE_COLLIDABLE_H

#include <vector>

#include "aabb.h"
#include "defines.h"

namespace util::collidable {

    class Collidable {
    public:
        auto collision(const Collidable &other) const -> bool {
            for (const auto &aabb : this->hitboxes) {
                for (const auto &_aabb : other.hitboxes) {
                    if (aabb.intersection(_aabb))
                        return true;
                }
            }

            return false;
        }

    private:
        std::vector<aabb::AABB<f32>> hitboxes;
    };
}

#endif //OPENGL_3D_ENGINE_COLLIDABLE_H
