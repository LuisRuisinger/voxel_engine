//
// Created by Luis Ruisinger on 02.09.24.
//

#include "dirt.h"

#include "../core/level/model/voxel.h"

namespace core::level::tiles::impl::dirt {
    Dirt::Dirt()  {
        this->flags ^= this->flags;
        this->flags |= 0x1'111111'0;
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