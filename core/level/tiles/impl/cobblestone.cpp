//
// Created by Luis Ruisinger on 02.09.24.
//

#include "cobblestone.h"

namespace core::level::tiles::impl::cobblestone {
    Cobblestone::Cobblestone() : tile::Tile() {
        this->flags = tile::can_cull_other | tile::can_cull_itself | tile::can_be_culled_by_other;
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