//
// Created by Luis Ruisinger on 05.04.24.
//

#include "Octree.h"

struct Helper {
    bool *visitXN;
    bool *visitXP;
    bool *visitYN;
    bool *visitYP;
    bool *visitZN;
    bool *visitZP;

    Helper()
        : visitXN{new bool[CHUNK_SIZE * CHUNK_SIZE]}
        , visitXP{new bool[CHUNK_SIZE * CHUNK_SIZE]}
        , visitYN{new bool[CHUNK_SIZE * CHUNK_SIZE]}
        , visitYP{new bool[CHUNK_SIZE * CHUNK_SIZE]}
        , visitZN{new bool[CHUNK_SIZE * CHUNK_SIZE]}
        , visitZP{new bool[CHUNK_SIZE * CHUNK_SIZE]}
    {}

    ~Helper() {
        delete[] visitXN;
        delete[] visitXP;
        delete[] visitYN;
        delete[] visitYP;
        delete[] visitZN;
        delete[] visitZP;
    }

    auto reset() -> void {
        std::fill_n(visitXN, CHUNK_SIZE * CHUNK_SIZE, false);
        std::fill_n(visitXP, CHUNK_SIZE * CHUNK_SIZE, false);
        std::fill_n(visitYN, CHUNK_SIZE * CHUNK_SIZE, false);
        std::fill_n(visitYP, CHUNK_SIZE * CHUNK_SIZE, false);
        std::fill_n(visitZN, CHUNK_SIZE * CHUNK_SIZE, false);
        std::fill_n(visitZP, CHUNK_SIZE * CHUNK_SIZE, false);
    };
};