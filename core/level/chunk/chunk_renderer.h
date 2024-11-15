//
// Created by Luis Ruisinger on 26.08.24.
//

#ifndef OPENGL_3D_ENGINE_CHUNK_RENDERER_H
#define OPENGL_3D_ENGINE_CHUNK_RENDERER_H

#include "../../util/defines.h"
#include "../../memory/linear_allocator.h"
#include "../../memory/arena_allocator.h"
#include "../../util/renderable.h"
#include "../platform.h"

#include "../core/level/tiles/tile_manager.h"


namespace core::level::chunk::chunk_renderer {
    template <typename T>
    struct Buffer {
        const T *mem;
        u64 capacity;
        u64 size;
    };

    using namespace core::memory;

    class ChunkRenderer : public util::renderable::Renderable<ChunkRenderer> {
    public:
        ChunkRenderer(arena_allocator::ArenaAllocator *, size_t);

        // renderable
        auto init() -> void;
        auto prepare_frame(state::State &state) -> void;
        auto frame(state::State &state) -> void;

        // write traversed voxels
        auto request_writeable_area(u64, u64) -> const VERTEX *;
        auto add_size_writeable_area(u64, u64) -> void;

    private:
        std::vector<std::vector<Buffer<VERTEX>>> storage;
        linear_allocator::LinearAllocator<arena_allocator::ArenaAllocator> allocator;
    };
}


#endif //OPENGL_3D_ENGINE_CHUNK_RENDERER_H