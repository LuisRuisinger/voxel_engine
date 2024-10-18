//
// Created by Luis Ruisinger on 13.10.24.
//

#include "water.h"

namespace core::level::tiles::impl::water {
    Water::Water() : tile::Tile() {
        this->flags = tile::can_cull_itself | tile::can_be_culled_by_other;
        this->mesh = std::make_shared<model::voxel::CubeStructure>();
        this->type = tile::WATER;
        this->textures = {
                "../resources/textures/default_cobblestone.png", // top
                "../resources/textures/default_cobblestone.png", // front
                "../resources/textures/default_cobblestone.png", // sides
                "../resources/textures/default_cobblestone.png"  // bottom
        };
    }
}