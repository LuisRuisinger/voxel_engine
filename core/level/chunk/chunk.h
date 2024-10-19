//
// Created by Luis Ruisinger on 18.02.24.
//

#ifndef OPENGL_3D_ENGINE_CHUNK_H
#define OPENGL_3D_ENGINE_CHUNK_H

#include <iostream>
#include <filesystem>
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <tuple>
#include <type_traits>

#include "../core/rendering/renderer.h"
#include "../core/level/chunk/chunk_segment.h"
#include "../core/level/chunk_data_structure/octree.h"

#include "../util/aliases.h"
#include "../util/result.h"

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

namespace core::state {
    struct State;
}

namespace core::level::chunk {
    enum Position : u8 {
        LEFT,
        RIGHT,
        FRONT,
        BACK
    };

    class OcclusionMap {
    public:
        OcclusionMap() =default;
        auto operator[](u64 mask) -> std::unordered_map<node::Node *, u32> &;

    private:
        std::array<std::unordered_map<node::Node *, u32>, 6> map;
    };

    class Chunk {
    public:
        Chunk(u16);
        ~Chunk() =default;

        Chunk(Chunk &&) =default;
        auto operator=(Chunk &&) -> Chunk & =default;

        auto generate(glm::vec2) -> void;

        template <rendering::renderer::RenderType R>
        auto insert(glm::vec3, u16, bool recombine = true) -> void;

        template <rendering::renderer::RenderType R>
        auto remove(glm::vec3) -> void;

        auto cull(state::State &) const -> void;
        auto update_and_render(u16, state::State &) -> void;

        auto find(glm::vec3) -> node::Node *;

        template <rendering::renderer::RenderType R>
        auto find(std::function<f32(const glm::vec3 &, const u32)> &) -> f32;

        auto update_occlusion(node::Node *, node::Node *, u64, u64) -> void;
        auto visible(const util::camera::Camera &, const glm::vec2 &) const -> bool;
        auto index() const -> u16;
        auto add_neigbor(Position, std::shared_ptr<Chunk>) -> void;
        auto recombine() -> void;


        struct Faces {
            auto operator[](u64 mask) -> u64 &;
            std::array<u64, 6> stored_faces;
        };

    private:
        std::vector<std::pair<Position, std::weak_ptr<Chunk>>> neighbors;
        OcclusionMap occlusion_map;

        std::vector<ChunkSegment> chunk_segments;

        u16 chunk_idx;
        u16 faces;
        u32 size;

        mutable size_t cur_frame_alloc_size { 0 };
        mutable Faces mask_container;

        u32 water_size { 0 };
    };
}


#endif //OPENGL_3D_ENGINE_CHUNK_H
