//
// Created by Luis Ruisinger on 02.09.24.
//

#ifndef OPENGL_3D_ENGINE_TILE_H
#define OPENGL_3D_ENGINE_TILE_H

#include "../model/voxel.h"
#include "../util/defines.h"
#include "../util/collidable.h"

#include <bitset>

namespace core::level::tiles::tile {
    using namespace util;

    const constexpr u8 can_cull_other = 1 << 7;
    const constexpr u8 can_be_culled_by_other = 1 << 6;
    const constexpr u8 can_cull_itself = 1 << 5;

    enum Type : u16 {
        GRASS = 0,
        DIRT  = 1,
        COBBLESTONE = 2,
        STONE = 3,
        STONE_BLOCK = 4,
        STONE_BRICK = 5,
        WATER = 6
    };

    struct Tile : public collidable::Collidable {
        u8 flags { 0 };
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

        auto can_cull(const Tile &other) const -> bool {
            return
                ((this->type == other.type) && (this->flags & can_cull_itself)) ||
                ((this->type != other.type) && (this->flags & can_cull_other) &&
                 (other.flags & can_be_culled_by_other));
        }
    };
}


#endif //OPENGL_3D_ENGINE_TILE_H
