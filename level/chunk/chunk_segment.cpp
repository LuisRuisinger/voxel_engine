//
// Created by Luis Ruisinger on 18.05.24.
//

#include "chunk_segment.h"

namespace core::level::chunk {
    ChunkSegment::ChunkSegment(u8 segmentIdx)
        : _segmentIdx{segmentIdx}
        , _modified{false}
        , _segment{std::make_unique<octree::Octree>()}
    {}

    ChunkSegment::ChunkSegment(ChunkSegment &&other) noexcept
        : _segmentIdx{other._segmentIdx}
        , _modified{other._modified}
        , _segment{std::move(other._segment)}
    {}

    auto ChunkSegment::operator=(ChunkSegment &&other) noexcept -> ChunkSegment & {
        _segmentIdx = other._segmentIdx;
        _modified   = other._modified;
        _segment    = std::move(other._segment);

        other._modified = false;
        return *this;
    }
}