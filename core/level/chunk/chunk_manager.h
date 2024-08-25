//
// Created by Luis Ruisinger on 27.06.24.
//

#ifndef OPENGL_3D_ENGINE_CHUNK_MANAGER_H
#define OPENGL_3D_ENGINE_CHUNK_MANAGER_H

#include "chunk.h"

#include <stdlib.h>

namespace core::level::chunk::chunk_manager {
    struct Chunkmanager {
        std::vector<std::shared_ptr<chunk::Chunk>> chunks;
        std::mutex                                 mutex;
    };
}

#endif //OPENGL_3D_ENGINE_CHUNK_MANAGER_H
