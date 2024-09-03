//
// Created by Luis Ruisinger on 03.09.24.
//

#ifndef OPENGL_3D_ENGINE_STONE_BRICK_H
#define OPENGL_3D_ENGINE_STONE_BRICK_H

#include "../core/level/tiles/tile.h"

namespace core::level::tiles::impl::stone_brick {
    struct Stonebrick : public tile::Tile {
        Stonebrick();
        ~Stonebrick() override =default;
    };
}

#endif //OPENGL_3D_ENGINE_STONE_BRICK_H
