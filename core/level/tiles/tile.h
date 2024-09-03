//
// Created by Luis Ruisinger on 02.09.24.
//

#ifndef OPENGL_3D_ENGINE_TILE_H
#define OPENGL_3D_ENGINE_TILE_H

#include "../model/voxel.h"
#include "../util/aliases.h"
#include "../util/collidable.h"

#include <bitset>

namespace core::level::tiles::tile {
    using namespace util;

    enum Type : u16 {
        GRASS = 0,
        DIRT  = 1,
        COBBLESTONE = 2,
        STONE = 3,
        STONE_BLOCK = 4,
        STONE_BRICK = 5
    };

    struct Tile : public collidable::Collidable {
        std::bitset<8> flags;
        std::shared_ptr<model::voxel::CubeStructure> mesh;
        Type type;
        std::string texture;
        std::array<std::string, 4> textures;

        Tile() =default;
        virtual ~Tile() =default;
        virtual auto on_place(glm::vec3) -> void {};
        virtual auto set_state(glm::vec3) -> void {};
        virtual auto on_destroy(glm::vec3) -> void {};
        virtual auto emit(glm::vec3) -> void {};
    };
}


#endif //OPENGL_3D_ENGINE_TILE_H
