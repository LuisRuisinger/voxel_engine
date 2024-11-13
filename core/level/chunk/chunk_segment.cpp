//
// Created by Luis Ruisinger on 18.05.24.
//

#include "chunk_segment.h"

namespace core::level::chunk {
    ChunkSegment::ChunkSegment(u8 segmentIdx)
        : voxel_root     { std::make_unique<octree::Octree>() },
          water_root     { std::make_unique<octree::Octree>() },
          chunk_modified { false                              },
          segment_idx    { segmentIdx                         }
    {}

    ChunkSegment::ChunkSegment(ChunkSegment &&other) noexcept
        : voxel_root     { std::move(other.voxel_root)                       },
          water_root     { std::move(other.water_root)                       },
          chunk_modified { other.chunk_modified                              },
          initialized    { other.initialized.load(std::memory_order_acquire) },
          segment_idx    { other.segment_idx                                 }
    {
        other.chunk_modified = false;
    }

    auto ChunkSegment::operator=(ChunkSegment &&other) noexcept -> ChunkSegment & {
        this->segment_idx = other.segment_idx;
        this->chunk_modified = other.chunk_modified;
        this->initialized = other.initialized.load(std::memory_order_acquire);
        this->voxel_root = std::move(other.voxel_root);
        this->water_root = std::move(other.water_root);

        other.chunk_modified = false;
        return *this;
    }
}