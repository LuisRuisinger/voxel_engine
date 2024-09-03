//
// Created by Luis Ruisinger on 31.08.24.
//

#include "generation.h"

#include "../../../util/log.h"
#include "../chunk.h"
#include "biome.h"

#define COMPRESS_1 (256 * 2)
#define COMPRESS_2 (128 * 2)
#define COMPRESS_3 (256)
#define COMPRESS_4 (128 * 2)

namespace core::level::chunk::generation::generation {
    constexpr const i32 deviation = CHUNK_SIZE * 10;

    siv::BasicPerlinNoise<f32> noise1 { std::mt19937 { 1234 } };
    siv::BasicPerlinNoise<f32> noise2 { std::mt19937 { 4321 } };

    auto Generator::generate(chunk::Chunk &chunk, glm::vec2 offset) -> void {
        for (auto x = 0; x < CHUNK_SIZE; ++x) {
            for (auto z = 0; z < CHUNK_SIZE; ++z) {
                f32 nx = x + offset.x;
                f32 nz = z + offset.y;

                f32 e =
                        1.0  * noise1.noise2D( 1 * nx / COMPRESS_1,  1 * nz / COMPRESS_1) +
                        0.75 * noise1.noise2D( 2 * nx / COMPRESS_1,  2 * nz / COMPRESS_1) +
                        0.25 * noise1.noise2D( 4 * nx / COMPRESS_2,  4 * nz / COMPRESS_2) +
                        0.13 * noise1.noise2D( 8 * nx / COMPRESS_2,  8 * nz / COMPRESS_2) +
                        0.06 * noise1.noise2D(16 * nx / COMPRESS_3, 16 * nz / COMPRESS_3) +
                        0.03 * noise1.noise2D(32 * nx / COMPRESS_4, 32 * nz / COMPRESS_4);

                e = e / (1.0F + 0.75F + 0.25F + 0.13F + 0.06F + 0.03F);

                f32 m =
                        0.75 * noise2.noise2D( 1 * nx / COMPRESS_1,  1 * nz / COMPRESS_1) +
                        0.75 * noise2.noise2D( 2 * nx / COMPRESS_1,  2 * nz / COMPRESS_1) +
                        0.33 * noise2.noise2D( 4 * nx / COMPRESS_2,  4 * nz / COMPRESS_2) +
                        0.05 * noise2.noise2D( 8 * nx / COMPRESS_4,  8 * nz / COMPRESS_2) +
                        0.05 * noise2.noise2D(16 * nx / COMPRESS_4, 16 * nz / COMPRESS_4) +
                        0.05 * noise2.noise2D(32 * nx / COMPRESS_4, 32 * nz / COMPRESS_4);

                m = m / (0.75F + 0.75F + 0.33F + 0.05F + 0.05F + 0.05F);
                m = (e + m) / 2.0F;
                m = m * deviation;

                if (m <= 25) {
                    biome::Cliffs().generate(chunk, x, z, m);
                }
                else {
                    biome::Forest().generate(chunk, x, z, m);
                }
            }
        }
    }
}