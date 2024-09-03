//
// Created by Luis Ruisinger on 02.09.24.
//

#ifndef OPENGL_3D_ENGINE_COBBLESTONE_H
#define OPENGL_3D_ENGINE_COBBLESTONE_H

#include "../core/level/tiles/tile.h"

namespace core::level::tiles::impl::cobblestone {
    struct Cobblestone : public tile::Tile {
        Cobblestone();
        ~Cobblestone() override =default;
    };
}

#endif //OPENGL_3D_ENGINE_COBBLESTONE_H
