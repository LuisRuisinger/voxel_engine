//
// Created by Luis Ruisinger on 31.08.24.
//

#ifndef OPENGL_3D_ENGINE_GENERATION_H
#define OPENGL_3D_ENGINE_GENERATION_H

#include <glm/vec2.hpp>

#include "../util/defines.h"
#include "../util/perlin_noise.hpp"


namespace core::level::chunk {
    class Chunk;
}

namespace core::level::chunk::generation::generation {
    enum Biome : u8 {
        OCEAN,
        BEACH,
        PLAINS
    };

    struct Generator {
        static auto generate(chunk::Chunk &, glm::vec2 offset) -> void;
    };
}


#endif //OPENGL_3D_ENGINE_GENERATION_H
