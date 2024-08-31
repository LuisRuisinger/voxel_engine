//
// Created by Luis Ruisinger on 17.02.24.
//

#ifndef OPENGL_3D_ENGINE_OCTREE_H
#define OPENGL_3D_ENGINE_OCTREE_H

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <memory>
#include <array>
#include <vector>
#include <stack>
#include <functional>

#include "../../../util/aliases.h"
#include "glad/glad.h"
#include "node_inline.h"
#include "node.h"

namespace core::level::octree {

    class Octree {
    public:
        Octree() =default;
        ~Octree() = default;

        auto addPoint(u64) -> node::Node *;
        auto removePoint(u16) -> void;
        auto cull(
                const glm::vec3 &,
                const util::camera::Camera &,
                state::State &,
                const VERTEX *,
                u64 &) const
                -> void;
        auto find(u32) const -> node::Node *;
        auto find(
                const glm::vec3 &,
                std::function<f32(const glm::vec3 &, const u32)> &) -> f32;
        auto updateFaceMask(u16) -> u8;
        auto update_chunk_mask(u16) -> void;
        auto recombine() -> void;
        auto count_mask(u64) -> size_t;

    private:

        /** @brief Root of the chunk_data_structure */
        std::unique_ptr<node::Node> _root = std::make_unique<node::Node>();

        /** @brief Sets the base bounding volume for an chunk_data_structure */
        const u32 _packed = (0x3F << 18) | (0x10 << 13) | (0x10 << 8) | (0x10 << 3) | 5;
    };
}

#endif //OPENGL_3D_ENGINE_OCTREE_H