//
// Created by Luis Ruisinger on 02.09.24.
//

#ifndef OPENGL_3D_ENGINE_BIOME_H
#define OPENGL_3D_ENGINE_BIOME_H

#include "../../../util/defines.h"
#include "../../../util/perlin_noise.hpp"

#define WATER_LEVEL 64

namespace core::level::chunk {
    class Chunk;
}

namespace core::level::chunk::generation::biome {

    template <typename T>
    struct Biome {
        auto generate(chunk::Chunk &chunk, f32 x, f32 z, f32 m) -> void {
            static_cast<T *>(this)->generate(chunk, x, z, m);
        }

        auto amplify(f32) -> f32 {
            return static_cast<T *>(this)->amplify();
        }

        auto step(f32 i, f32 a, f32 b) -> f32 {
            return static_cast<f32>(std::round(i * static_cast<f32>(a))) / static_cast<f32>(b);
        }
    };

    struct Forest : public Biome<Forest> {
        auto generate(chunk::Chunk &chunk, f32 x, f32 z, f32 m) -> void;
        auto amplify(f32) -> f32;
    };

    struct Cliffs : public Biome<Forest> {
        auto generate(chunk::Chunk &chunk, f32 x, f32 z, f32 m) -> void;
        auto amplify(f32) -> f32;
    };
}

#endif //OPENGL_3D_ENGINE_BIOME_H
