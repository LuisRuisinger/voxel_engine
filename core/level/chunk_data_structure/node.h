//
// Created by Luis Ruisinger on 26.05.24.
//

#ifndef OPENGL_3D_ENGINE_NODE_H
#define OPENGL_3D_ENGINE_NODE_H

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <memory>
#include <array>
#include <vector>
#include <stack>
#include <functional>

#include "../../../util/defines.h"
#include "glad/glad.h"
#include "../../../util/culling.h"
#include "../../../util/camera.h"

namespace core::state {
    class State;
}

namespace core::level::node {
    struct Args {
        const glm::ivec3 &_point;
        const util::camera::Camera &_camera;
        const VERTEX *_voxelVec;
        u64 &actual_size;
    };

    struct Node {
        Node() =default;
        ~Node() =default;

        Node(Node &&) noexcept =default;
        auto operator=(Node &&) noexcept -> Node & =default;

        Node(const Node &) =delete;
        auto operator=(const Node &) =delete;

        auto cull(Args &, util::culling::CollisionType type) const -> void;
        auto update_face_mask(u16) -> u8;
        auto recombine() -> void;
        auto update_chunk_mask(u16) -> void;
        auto count_mask(u64) -> size_t;
        auto find_node(
                const glm::vec3 &,
                std::function<f32(const glm::vec3 &, const u32)> &) -> f32;

        std::unique_ptr<std::array<Node, 8>> nodes {};
        u64 packed_data { 0 };

        // segments: 8 | faces: 6 | curX: 5 | curY: 5 | curZ: 5 | scale: 3 (exactly 32)
        // chunkIndex2D: 12 | chunkSegmentOffsetY: 4 | unused: 8 | voxelID: 8 (exactly 32)

        // unused: 14 | curX: 5 | curY: 5 | curZ: 5 | scale: 3 (exactly 32)
        // chunkIndex2D: 12 | chunkSegmentOffsetY: 4 | unused: 7 | voxelID: 9 (exactly 32)

        // offsetX: 3 | offsetY: 3 | offsetZ: 3 | uv: 4 | unused: 1 | curX: 5 | curY: 5 | curZ: 5 | scale: 3 (exactly 32)
        // chunkIndex2D: 12 | chunkSegmentOffsetY: 4 | normals: 3 | unused: 4 | voxelID: 9 (exactly 32)

    };

    static_assert(sizeof(Node) == 2 * sizeof(Node *));

    inline auto insert_node(u64, u32, Node *) -> Node *;
    inline auto find_node(u32, Node *) -> Node *;
}

#endif //OPENGL_3D_ENGINE_NODE_H
