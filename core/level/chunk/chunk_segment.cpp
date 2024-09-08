//
// Created by Luis Ruisinger on 18.05.24.
//

#include "chunk_segment.h"

namespace core::level::chunk {
    ChunkSegment::ChunkSegment(u8 segmentIdx)
        : segment_idx    { segmentIdx                         }
        , chunk_modified { false                              }
        , root           { std::make_unique<octree::Octree>() }
    {}

    ChunkSegment::ChunkSegment(ChunkSegment &&other) noexcept
        : segment_idx    { other.segment_idx                                 }
        , chunk_modified { other.chunk_modified                              }
        , initialized    { other.initialized.load(std::memory_order_acquire) }
        , root           { std::move(other.root)                             }
    {
        other.chunk_modified = false;
    }

    auto ChunkSegment::operator=(ChunkSegment &&other) noexcept -> ChunkSegment & {
        this->segment_idx = other.segment_idx;
        this->chunk_modified = other.chunk_modified;
        this->initialized = other.initialized.load(std::memory_order_acquire);
        this->root = std::move(other.root);

        other.chunk_modified = false;
        return *this;
    }
}