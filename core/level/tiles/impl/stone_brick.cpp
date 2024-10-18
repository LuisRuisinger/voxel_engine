//
// Created by Luis Ruisinger on 03.09.24.
//

#include "stone_brick.h"

namespace core::level::tiles::impl::stone_brick {
    Stonebrick::Stonebrick() : tile::Tile() {
        this->flags = tile::can_cull_other | tile::can_cull_itself | tile::can_be_culled_by_other;
        this->mesh = std::make_shared<model::voxel::CubeStructure>();
        this->type = tile::STONE_BRICK;
        this->textures = {
                "../resources/textures/default_stone_brick.png", // top
                "../resources/textures/default_stone_brick.png", // front
                "../resources/textures/default_stone_brick.png", // sides
                "../resources/textures/default_stone_brick.png",  // bottom
        };
    }
}