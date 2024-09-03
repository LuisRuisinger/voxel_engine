//
// Created by Luis Ruisinger on 02.09.24.
//

#ifndef OPENGL_3D_ENGINE_GRASS_H
#define OPENGL_3D_ENGINE_GRASS_H

#include "../tile.h"

namespace core::level::tiles::impl::grass {
    struct Grass : public tile::Tile {
        Grass();
        ~Grass() override =default;
    };
}


#endif //OPENGL_3D_ENGINE_GRASS_H
