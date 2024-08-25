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

#include "../../../util/aliases.h"
#include "glad/glad.h"
#include "../../rendering/renderer.h"

namespace core::level::platform {
    class Platform;
}

namespace core::level::node {
    struct Args {
        const glm::vec3 &_point;
        const util::camera::Camera &_camera;
        platform::Platform &_platform;
        const VERTEX *_voxelVec;
        u64 &actual_size;
    };

    struct Node {
        Node();
        ~Node() =default;

        Node(Node &&) noexcept =default;
        auto operator=(Node &&) noexcept -> Node & =default;

        Node(const Node &) =delete;
        auto operator=(const Node &) =delete;

        auto cull(Args &, util::culling::CollisionType type) const -> void;
        auto updateFaceMask(u16) -> u8;
        auto recombine() -> void;
        auto update_chunk_mask(u16) -> void;
        auto count_mask(u64) -> size_t;

        std::unique_ptr<std::array<Node, 8>> nodes {};
        u64 packed_data { 0 };

        // segments: 8 | faces: 6 | curX: 5 | curY: 5 | curZ: 5 | scale: 3 (exactly 32)
        // chunkIndex2D: 12 | chunkSegmentOffsetY: 4 | unused: 8 | voxelID: 8 (exactly 32)

        // unused: 14 | curX: 5 | curY: 5 | curZ: 5 | scale: 3 (exactly 32)
        // chunkIndex2D: 12 | chunkSegmentOffsetY: 4 | unused: 8 | voxelID: 8 (exactly 32)

        // offsetX: 3 | offsetY: 3 | offsetZ: 3 | uv: 4 | unused: 1 | curX: 5 | curY: 5 | curZ: 5 | scale: 3 (exactly 32)
        // chunkIndex2D: 12 | chunkSegmentOffsetY: 4 | normals: 3 | unused: 5 | voxelID: 8 (exactly 32)
    };

    static_assert(sizeof(Node) == 2 * sizeof(Node *));

    inline auto insertNode(u64 packedVoxel, u32 packedData, Node *current) -> Node *;
    inline auto findNode(u32 packedVoxel, Node *current) -> Node *;
}

#endif //OPENGL_3D_ENGINE_NODE_H
