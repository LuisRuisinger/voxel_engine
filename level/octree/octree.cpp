//
// Created by Luis Ruisinger on 17.02.24.
//

#include <immintrin.h>
#include <cmath>

#include "octree.h"

namespace core::level::octree {

    // ----------------------
    // Octree implementation

    Octree::Octree()
        : _root{std::make_unique<node::Node>()}
    {}

    auto Octree::addPoint(u64 packedVoxel) -> node::Node * {
        return node_inline::insertNode(packedVoxel, this->_packed, this->_root.get());
    }

    auto Octree::removePoint(u16 position) -> void {}

    auto Octree::cull(const glm::vec3 &position,
                      const camera::perspective::Camera &camera,
                      Platform &platform,
                      std::vector<VERTEX> &voxelVec) const -> void {
        const node::Args args = {position, camera, platform, voxelVec};
        this->_root->cull(args, camera::culling::INTERSECT);
    }

    auto Octree::find(u32 packedVoxel) const -> std::optional<node::Node *> {
        return node_inline::findNode(packedVoxel, _root.get());
    }

    auto Octree::updateFaceMask(u16 mask) -> u8 {
        return this->_root->updateFaceMask(mask);
    }

    auto Octree::recombine() -> void {
        std::stack<node::Node *> stack;
        this->_root->recombine(stack);
    }
}