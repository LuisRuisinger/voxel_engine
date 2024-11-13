//
// Created by Luis Ruisinger on 18.02.24.
//

#ifndef OPENGL_3D_ENGINE_CHUNK_H
#define OPENGL_3D_ENGINE_CHUNK_H

#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <tuple>
#include <type_traits>

#include "../core/rendering/renderer.h"
#include "../core/level/chunk/chunk_segment.h"
#include "../core/level/chunk_data_structure/octree.h"

#include "../util/defines.h"
#include "../util/result.h"

#define CHUNK_SEGMENT_YOFFS(y) \
    (CHUNK_SIZE * y + MIN_HEIGHT)

#define CHUNK_SEGMENT_Y_NORMALIZE(_p)          \
    glm::ivec3 {                                \
        (_p).x,                                \
        static_cast<i32>((_p).y) % CHUNK_SIZE, \
        (_p).z                                 \
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

        auto generate(glm::ivec2) -> void;

        template <rendering::renderer::RenderType R>
        auto insert(glm::ivec3, u16, bool recombine = true) -> void;

        template <rendering::renderer::RenderType R>
        auto remove(glm::ivec3) -> void;

        auto cull(state::State &) const -> void;
        auto update_and_render(u16, state::State &) -> void;

        auto find(glm::ivec3) -> node::Node *;
        auto find(std::function<f32(const glm::vec3 &, const u32)> &) -> f32;

        auto update_occlusion(node::Node *, node::Node *, u64, u64) -> void;
        auto visible(const util::camera::Camera &, const glm::ivec2 &) const -> bool;
        auto index() const -> u16;
        auto add_neigbor(Position, std::shared_ptr<Chunk>) -> void;
        auto recombine() -> void;

    private:
        std::vector<std::pair<Position, std::weak_ptr<Chunk>>> neighbors;
        OcclusionMap occlusion_map;

        std::vector<ChunkSegment> chunk_segments;

        glm::ivec3 chunk_pos;
        u16 chunk_idx;
        u16 faces;

        u32 voxel_size { 0 };
        u32 water_size { 0 };
    };
}


#endif //OPENGL_3D_ENGINE_CHUNK_H
