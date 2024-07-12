//
// Created by Luis Ruisinger on 18.02.24.
//

#ifndef OPENGL_3D_ENGINE_CHUNK_H
#define OPENGL_3D_ENGINE_CHUNK_H

#include <iostream>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <tuple>
#include <type_traits>

#include "../../util/aliases.h"
#include "../level/octree/octree.h"
#include "../level/chunk/chunk_segment.h"

#define CHUNK_SEGMENT_YOFFS(y) \
    (CHUNK_SIZE * y + MIN_HEIGHT)

#define CHUNK_SEGMENT_Y_NORMALIZE(_p)                            \
    glm::vec3 {                                                  \
        (_p).x,                                                  \
        static_cast<f32>(static_cast<i32>((_p).y) % CHUNK_SIZE), \
        (_p).z                                                   \
    }

#define CHUNK_SEGMENT_Y_DIFF(_p) \
    ((static_cast<i32>((_p).y) + abs(MIN_HEIGHT)) / CHUNK_SIZE)

namespace core::level {
    class Platform;
}

namespace core::level::chunk {
    enum ChunkData {
        EMPTY, NODATA, DATA
    };

    //
    //
    //

    class Chunk {
    public:
        Chunk(u16);
        ~Chunk() =default;

        Chunk(Chunk &&) =default;
        auto operator=(Chunk &&) -> Chunk & =default;

        auto generate(Platform *) -> void;
        auto insert(glm::vec3, u8, Platform *platform) -> void;
        auto remove(glm::vec3) -> void;
        auto cull(const core::camera::perspective::Camera &, Platform &) const -> void;
        auto update_and_render(u16, const core::camera::perspective::Camera &, Platform &) -> void;

        auto find(glm::vec3, Platform *platform) -> std::pair<node::Node *, ChunkData>;
        auto updateOcclusion(node::Node *, std::pair<node::Node *, ChunkData>, u64, u64) -> void;
        auto visible(const camera::perspective::Camera &, const Platform &) const -> bool;
        auto index() const -> u16;

    private:

        // ------------------------------------------------------
        // a vector of cubic segments which a chunk is split into

        std::vector<ChunkSegment> _chunksegments;
        u16                       _chunkIdx;

        // used for determing if enqueue in threadpool needed
        u32 _size;
        u8  _faces;
    };
}


#endif //OPENGL_3D_ENGINE_CHUNK_H