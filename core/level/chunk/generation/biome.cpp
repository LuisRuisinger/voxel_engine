//
// Created by Luis Ruisinger on 02.09.24.
//

#include "biome.h"
#include "../chunk.h"
#include "../chunk_renderer.h"

namespace core::level::chunk::generation::biome {
    constexpr const i32 deviation = CHUNK_SIZE * 10;
    constexpr const i32 max_height = HEIGHT_01 + MIN_HEIGHT;

    static siv::BasicPerlinNoise<f32> noise1 { std::mt19937 { 1234 } };
    static siv::BasicPerlinNoise<f32> noise2 { std::mt19937 { 4321 } };

    auto Cliffs::generate(chunk::Chunk &chunk, f32 x, f32 z, f32 m) -> void {
        m = amplify(m);
        m = step(m, 0.6F, 0.6F);

        auto max_y = std::min(static_cast<i32>(m + WATER_LEVEL), max_height);
        for (auto y = max_y + 1; y < WATER_LEVEL; ++y) {
            auto pos = glm::vec3{x, y, z};
            chunk.insert<rendering::renderer::RenderType::WATER_RENDERER>(
                    pos, tiles::tile::WATER, nullptr, false);
        }

        for (auto y = 0; y <= max_y; ++y) {
            auto pos = glm::vec3{x, y, z};
            chunk.insert<rendering::renderer::RenderType::CHUNK_RENDERER>(
                    pos, tiles::tile::STONE, nullptr, false);
        }
    }

    auto Cliffs::amplify(f32 i) -> f32 {
        auto ret = std::atan(std::pow(i / 2.0F, 3.0F));
        ret = std::pow(ret, 3.0F) * 1.25F;
        ret = ret * std::log(std::pow(i, 2.0F));

        return ret + 1;
    }

    auto Forest::generate(chunk::Chunk &chunk, f32 x, f32 z, f32 m) -> void {
        m = amplify(m);
        m = step(m, 2.0F, 2.0F);

        auto max_y = std::min(static_cast<i32>(m + WATER_LEVEL), max_height);
        for (auto y = max_y - 3; y <= max_y; ++y) {
            auto pos = glm::vec3 { x, y, z };
            chunk.insert<rendering::renderer::RenderType::CHUNK_RENDERER>(
                    pos, tiles::tile::GRASS, nullptr, false);
        }

        for (auto y = 0; y < max_y - 3; ++y) {
            auto pos = glm::vec3 { x, y, z };
            chunk.insert<rendering::renderer::RenderType::CHUNK_RENDERER>(
                    pos, tiles::tile::STONE, nullptr, false);
        }
    }

    auto Forest::amplify(f32 i) -> f32 {
        auto ret = (i - 25.0F) / 16.0F;
        ret = std::pow(ret, 2.0F) + 32.0F;

        return ret - 1;
    }

}
