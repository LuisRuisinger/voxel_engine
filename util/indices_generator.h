//
// Created by Luis Ruisinger on 25.04.24.
//

#ifndef OPENGL_3D_ENGINE_INDICES_GENERATOR_H
#define OPENGL_3D_ENGINE_INDICES_GENERATOR_H

#include "aliases.h"

#define INDICES_PER_FACE 6

namespace util {
    template<unsigned N>
    struct IndicesGenerator {
        IndicesGenerator()
            : arr{}
        {
            u32 genIdx = 0;

            for (u32 i = 0; i < (N * INDICES_PER_FACE) - INDICES_PER_FACE; i += INDICES_PER_FACE) {
                arr[i] = genIdx;
                arr[i + 1] = genIdx + 1;
                arr[i + 2] = genIdx + 3;
                arr[i + 3] = genIdx + 1;
                arr[i + 4] = genIdx + 2;
                arr[i + 5] = genIdx + 3;

                genIdx += 4;
            }
        }

        constexpr auto end() -> u32 * {
            return &this->arr[N * INDICES_PER_FACE];
        }

        u32 arr[N * INDICES_PER_FACE];
    };
}

#endif //OPENGL_3D_ENGINE_INDICES_GENERATOR_H
