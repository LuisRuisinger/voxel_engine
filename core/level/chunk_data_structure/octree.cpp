//
// Created by Luis Ruisinger on 17.02.24.
//

#include <immintrin.h>
#include <cmath>

#include "octree.h"

namespace core::level::octree {

    auto Octree::addPoint(u64 packedVoxel) -> node::Node * {
        return node_inline::insertNode(packedVoxel, this->_packed, this->_root.get());
    }

    auto Octree::removePoint(u16 position) -> void {}

    auto Octree::cull(
            const glm::vec3 &position,
            const util::camera::Camera &camera,
            state::State &state,
            const VERTEX *voxelVec,
            u64 &actual_size) const
            -> void {
        node::Args args = {
                position, camera, state, voxelVec, actual_size
        };
        this->_root->cull(args, util::culling::INTERSECT);
    }

    auto Octree::find(u32 packedVoxel) const -> node::Node * {
        return node_inline::findNode(packedVoxel, _root.get());
    }

    auto Octree::find(
            const glm::vec3 &chunk_pos,
            std::function<f32(const glm::vec3 &, const u32)> &fun) -> f32 {
        return this->_root->find_node(chunk_pos, fun);
    }

    auto Octree::updateFaceMask(u16 mask) -> u8 {
        return this->_root->updateFaceMask(mask);
    }

    auto Octree::recombine() -> void {
        this->_root->recombine();
    }

    auto Octree::update_chunk_mask(u16 mask) -> void {
        this->_root->update_chunk_mask(mask);
    }

    auto Octree::count_mask(u64 mask) -> size_t {
        return this->_root->count_mask(mask);
    }
}