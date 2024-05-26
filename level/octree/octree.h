//
// Created by Luis Ruisinger on 17.02.24.
//

#ifndef OPENGL_3D_ENGINE_OCTREE_H
#define OPENGL_3D_ENGINE_OCTREE_H

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <memory>
#include <array>
#include <vector>
#include <stack>
#include <functional>

#include "../../util/aliases.h"
#include "glad/glad.h"
#include "node_inline.h"
#include "node.h"
#include "../Model/mesh.h"
#include "../../rendering/renderer.h"

namespace core::level::octree {

    class Octree {
    public:
        Octree();
        ~Octree() = default;

        auto addPoint(u64) -> node::Node *;
        auto removePoint(u16) -> void;
        auto cull(const glm::vec3 &,
                  const camera::perspective::Camera &,
                  const rendering::Renderer &,
                  std::vector<VERTEX> &) const -> void;
        auto find(u32) const -> std::optional<node::Node *>;
        auto updateFaceMask(u16) -> u8;
        auto recombine() -> void;

    private:
        std::unique_ptr<node::Node>  _root;

        // sets the base bounding volume for an octree
        const u32 _packed = (0x3F << 18) | (0x10 << 13) | (0x10 << 8) | (0x10 << 3) | 5;

    };
}

#endif //OPENGL_3D_ENGINE_OCTREE_H