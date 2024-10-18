//
// Created by Luis Ruisinger on 02.09.24.
//

#include "dirt.h"

#include "../core/level/model/voxel.h"

namespace core::level::tiles::impl::dirt {
    Dirt::Dirt() : tile::Tile()  {
        this->flags = tile::can_cull_other | tile::can_cull_itself | tile::can_be_culled_by_other;
        this->mesh = std::make_shared<model::voxel::CubeStructure>();
        this->type = tile::DIRT;
        this->textures = {
                "../resources/textures/default_dirt.png", // top
                "../resources/textures/default_dirt.png", // bottom
                "../resources/textures/default_dirt.png", // front
                "../resources/textures/default_dirt.png"  // everything else
        };
    }
}