//
// Created by Luis Ruisinger on 02.09.24.
//

#ifndef OPENGL_3D_ENGINE_DIRT_H
#define OPENGL_3D_ENGINE_DIRT_H

#include "../core/level/tiles/tile.h"

namespace core::level::tiles::impl::dirt {
    struct Dirt : public tile::Tile {
        Dirt();
        ~Dirt() override =default;
    };
}

#endif //OPENGL_3D_ENGINE_DIRT_H
