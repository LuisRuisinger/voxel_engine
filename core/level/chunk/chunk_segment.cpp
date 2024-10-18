//
// Created by Luis Ruisinger on 18.05.24.
//

#include "chunk_segment.h"

namespace core::level::chunk {
    ChunkSegment::ChunkSegment(u8 segmentIdx)
        : segment_idx    { segmentIdx                         },
          chunk_modified { false                              },
          voxel_root     { std::make_unique<octree::Octree>() },
          water_root     { std::make_unique<octree::Octree>() }
    {}

    ChunkSegment::ChunkSegment(ChunkSegment &&other) noexcept
        : segment_idx    { other.segment_idx                                 },
          chunk_modified { other.chunk_modified                              },
          initialized    { other.initialized.load(std::memory_order_acquire) },
          voxel_root     { std::move(other.voxel_root)                       },
          water_root     { std::move(other.water_root)                       }
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