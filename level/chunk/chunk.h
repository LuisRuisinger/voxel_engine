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
#include "../../util/result.h"

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
    enum Position : u8 {
        LEFT,
        RIGHT,
        FRONT,
        BACK
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
        auto insert(glm::vec3, u8, Platform *platform, bool recombine = true) -> void;
        auto remove(glm::vec3) -> void;
        auto cull(const core::camera::perspective::Camera &, Platform &) const -> void;
        auto update_and_render(u16, const core::camera::perspective::Camera &, Platform &) -> void;

        auto find(glm::vec3, Platform *platform) -> node::Node *;
        auto updateOcclusion(node::Node *, node::Node *, u64, u64) -> void;
        auto visible(const camera::perspective::Camera &, const Platform &) const -> bool;
        auto index() const -> u16;
        auto add_neigbor(Position, std::shared_ptr<Chunk>) -> void;
        auto recombine() -> void;


        struct Faces {
            auto operator[](u64 mask) -> u64 &;
            std::array<u64, 6> stored_faces;
        };

    private:

        std::vector<std::pair<Position, std::weak_ptr<Chunk>>> neighbors {};
        std::vector<ChunkSegment> chunk_segments {};
        u16                       chunk_idx;

        u32 size { 0 };
        u8  faces { 0 };

        mutable size_t cur_frame_alloc_size { 0 };
        mutable Faces mask_container;
    };
}


#endif //OPENGL_3D_ENGINE_CHUNK_H
