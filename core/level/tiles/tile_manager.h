//
// Created by Luis Ruisinger on 02.09.24.
//

#ifndef OPENGL_3D_ENGINE_TILE_MANAGER_H
#define OPENGL_3D_ENGINE_TILE_MANAGER_H

#include "../util/defines.h"
#include "../util/stb_image.h"
#include "tile.h"


namespace core::level::tiles::tile_manager {
    class TileManager {
    public:
        TileManager() =default;

        auto init() -> TileManager &;
        auto add_tile(tile::Tile) -> TileManager &;
        auto finalize() -> void;

        auto operator[](u32) -> tile::Tile &;

        u32 texture_array;
    private:

        u32 current_layer;
        stbi_uc *fallback_texture;
        std::array<std::unique_ptr<tile::Tile>, 512> tiles;
    };

    auto setup(TileManager &) -> void;

    extern TileManager tile_manager;
}

#endif //OPENGL_3D_ENGINE_TILE_MANAGER_H
