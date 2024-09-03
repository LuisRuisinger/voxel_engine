//
// Created by Luis Ruisinger on 02.09.24.
//

#include "stone.h"

namespace core::level::tiles::impl::stone {
    Stone::Stone()  {
        this->flags ^= this->flags;
        this->flags |= 0x1'111111'0;
        this->mesh = std::make_shared<model::voxel::CubeStructure>();
        this->type = tile::STONE;
        this->textures = {
                "../resources/textures/default_stone.png", // top
                "../resources/textures/default_stone.png", // front
                "../resources/textures/default_stone.png", // sides
                "../resources/textures/default_stone.png"  // bottom
        };
    }
}