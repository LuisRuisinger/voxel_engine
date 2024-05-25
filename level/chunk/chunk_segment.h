//
// Created by Luis Ruisinger on 18.05.24.
//

#ifndef OPENGL_3D_ENGINE_CHUNK_SEGMENT_H
#define OPENGL_3D_ENGINE_CHUNK_SEGMENT_H

#include <iostream>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../../util/aliases.h"
#include "../Rendering/renderer.h"
#include "../Level/Octree/octree.h"
#include "../Level/Octree/memorypool.h"

namespace core::level {
    class Platform;
}

namespace core::level::chunk {
    class Chunk;
}

namespace core::level::chunk {
    class ChunkSegment {
        friend Chunk;

    public:
        explicit ChunkSegment(u8);
        ChunkSegment(ChunkSegment &&other) noexcept;
        ChunkSegment(ChunkSegment &other) = delete;

        ~ChunkSegment() = default;

        auto operator=(ChunkSegment &&other) noexcept -> ChunkSegment &;
        auto operator=(ChunkSegment &other) -> ChunkSegment & = delete;

    private:
        // -------------------------------------------
        // underlying octree, managing the cubic space

        std::unique_ptr<octree::Octree> _segment;

        // ------------------------------------------------------------
        // indicator if the segment got manipulated (for serialization)

        bool _modified;

        u8 _segmentIdx;
    };
}


#endif //OPENGL_3D_ENGINE_CHUNK_SEGMENT_H
