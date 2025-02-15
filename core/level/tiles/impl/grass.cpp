//
// Created by Luis Ruisinger on 02.09.24.
//

#include "grass.h"

namespace core::level::tiles::impl::grass {
    Grass::Grass() : tile::Tile() {
        this->flags = tile::can_cull_other | tile::can_cull_itself | tile::can_be_culled_by_other;
        this->mesh = std::make_shared<model::voxel::CubeStructure>();
        this->type = tile::GRASS;
        this->textures = {
                "../resources/textures/default_grass_top.png", // top
                "../resources/textures/default_grass_side.png", // front
                "../resources/textures/default_grass_side.png", // sides
                "../resources/textures/default_dirt.png"  // bottom
        };
    }
}