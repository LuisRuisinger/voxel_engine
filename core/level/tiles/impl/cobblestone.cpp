//
// Created by Luis Ruisinger on 02.09.24.
//

#include "cobblestone.h"

namespace core::level::tiles::impl::cobblestone {
    Cobblestone::Cobblestone() {
        this->flags ^= this->flags;
        this->flags |= 0x1'111111'0;
        this->mesh = std::make_shared<model::voxel::CubeStructure>();
        this->type = tile::COBBLESTONE;
        this->textures = {
                "../resources/textures/default_cobblestone.png", // top
                "../resources/textures/default_cobblestone.png", // front
                "../resources/textures/default_cobblestone.png", // sides
                "../resources/textures/default_cobblestone.png"  // bottom
        };
    }
}