//
// Created by Luis Ruisinger on 13.10.24.
//

#ifndef OPENGL_3D_ENGINE_WATER_H
#define OPENGL_3D_ENGINE_WATER_H

#include "../core/level/tiles/tile.h"

namespace core::level::tiles::impl::water {
    struct Water : public tile::Tile {
        Water();
        ~Water() override =default;
    };
}


#endif //OPENGL_3D_ENGINE_WATER_H
