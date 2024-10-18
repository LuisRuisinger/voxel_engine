//
// Created by Luis Ruisinger on 02.09.24.
//

#include <filesystem>

#include "tile_manager.h"

#include "../core/opengl/opengl_window.h"
#include "../core/opengl/opengl_verify.h"

#include "../core/level/tiles/impl/dirt.h"
#include "../core/level/tiles/impl/grass.h"
#include "../core/level/tiles/impl/cobblestone.h"
#include "../core/level/tiles/impl/stone.h"
#include "../core/level/tiles/impl/stone_brick.h"
#include "../core/level/tiles/impl/water.h"

#include "../util/assert.h"

#define TEXTURE_ARRAY_WIDTH      16
#define TEXTURE_ARRAY_HEIGHT     16
#define TEXTURE_ARRAY_LAYERS     2048
#define TEXTURE_ARRAY_MIP_LEVELS 1
#define TEXTURE_FALLBACK         "../resources/textures/default_dirt.png"

namespace core::level::tiles::tile_manager {
    auto TileManager::init() -> TileManager & {
        OPENGL_VERIFY(glGenTextures(1, &this->texture_array));
        OPENGL_VERIFY(glBindTexture(GL_TEXTURE_2D_ARRAY, this->texture_array));
        OPENGL_VERIFY(
                glTexImage3D(
                        GL_TEXTURE_2D_ARRAY,
                        0,
                        GL_RGBA,
                        TEXTURE_ARRAY_WIDTH,
                        TEXTURE_ARRAY_HEIGHT,
                        TEXTURE_ARRAY_LAYERS,
                        0,
                        GL_RGBA,
                        GL_UNSIGNED_BYTE,
                        nullptr));

        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        if (GLAD_GL_EXT_texture_filter_anisotropic) {
            GLfloat max_anisotropic_level;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anisotropic_level);

            max_anisotropic_level = std::min<f32>(max_anisotropic_level, 4.0F);
            OPENGL_VERIFY(glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_LOD_BIAS, 0.0F));
            OPENGL_VERIFY(
                    glTexParameterf(
                            GL_TEXTURE_2D,
                            GL_TEXTURE_MAX_ANISOTROPY_EXT,
                            max_anisotropic_level));
        }

        ASSERT_EQ(std::filesystem::exists(TEXTURE_FALLBACK));
        stbi_set_flip_vertically_on_load(true);

        i32 width, height, nr_channels;
        this->fallback_texture = stbi_load(TEXTURE_FALLBACK, &width, &height, &nr_channels, 0);

        ASSERT_EQ(this->fallback_texture);
        ASSERT_EQ(width == TEXTURE_ARRAY_WIDTH);
        ASSERT_EQ(height == TEXTURE_ARRAY_HEIGHT);
        return *this;
    }

    auto TileManager::add_tile(tile::Tile tile) -> TileManager & {
        this->current_layer = tile.type * 4;

        for (const auto &texture : tile.textures) {
            i32 width, height, nr_channels;
            auto data = stbi_load(texture.c_str(), &width, &height, &nr_channels, 0);

            if (!data) {
                data = this->fallback_texture;
                LOG(util::log::LOG_LEVEL_ERROR, stbi_failure_reason());
            }

            ASSERT_EQ(width == TEXTURE_ARRAY_WIDTH);
            ASSERT_EQ(height == TEXTURE_ARRAY_HEIGHT);
            glTexSubImage3D(
                    GL_TEXTURE_2D_ARRAY,
                    0,
                    0, 0, this->current_layer,
                    TEXTURE_ARRAY_WIDTH, TEXTURE_ARRAY_HEIGHT, 1,
                    GL_RGBA,
                    GL_UNSIGNED_BYTE,
                    data);

            if (data != this->fallback_texture)
                stbi_image_free(data);

            ++this->current_layer;
        }

        this->tiles[tile.type] = std::make_unique<tile::Tile>(std::move(tile));
        return *this;
    }

    auto TileManager::finalize() -> void {
        glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
        stbi_image_free(this->fallback_texture);
    }

    auto TileManager::operator[](u32 id) -> tile::Tile & {
        return *this->tiles[id];
    }

    auto setup(TileManager &manager) -> void {
        manager
            .init()
            .add_tile(impl::dirt::Dirt {})
            .add_tile(impl::grass::Grass {})
            .add_tile(impl::cobblestone::Cobblestone {})
            .add_tile(impl::stone::Stone {})
            .add_tile(impl::stone_brick::Stonebrick {})
            .add_tile(impl::water::Water {})
            .finalize();
    }

    TileManager tile_manager {};
}
