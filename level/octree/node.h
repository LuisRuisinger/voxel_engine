//
// Created by Luis Ruisinger on 26.05.24.
//

#ifndef OPENGL_3D_ENGINE_NODE_H
#define OPENGL_3D_ENGINE_NODE_H

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <memory>
#include <array>
#include <vector>
#include <stack>
#include <functional>

#include "../../util/aliases.h"
#include "glad/glad.h"
#include "../Model/mesh.h"
#include "../../rendering/renderer.h"
#include "../../util/tagged_ptr.h"

namespace core::level {
    class Platform;
}

namespace core::level::node {
    struct Args {
        const glm::vec3                   &_point;
        const camera::perspective::Camera &_camera;
        Platform                          &_platform;
        std::vector<VERTEX>               &_voxelVec;
    };

    struct Node {
        Node() =default;
        ~Node() =default;

        Node(Node &&) noexcept =default;
        auto operator=(Node &&) noexcept -> Node & =default;

        Node(const Node &) =delete;
        auto operator=(const Node &) =delete;

        auto cull(const Args &, camera::culling::CollisionType type) const -> void;
        auto updateFaceMask(u16) -> u8;
        auto recombine() -> void;
        auto update_chunk_mask(u16) -> void;

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
    inline auto findNode(u32 packedVoxel, Node *current) -> std::optional<Node *>;
}

#endif //OPENGL_3D_ENGINE_NODE_H
