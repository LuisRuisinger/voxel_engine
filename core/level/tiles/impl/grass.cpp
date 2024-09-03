//
// Created by Luis Ruisinger on 02.09.24.
//

#include "grass.h"

namespace core::level::tiles::impl::grass {
    Grass::Grass()  {
        this->flags ^= this->flags;
        this->flags |= 0x1'111111'0;
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