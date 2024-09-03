//
// Created by Luis Ruisinger on 26.08.24.
//

#ifndef OPENGL_3D_ENGINE_CHUNK_RENDERER_H
#define OPENGL_3D_ENGINE_CHUNK_RENDERER_H

#include "../../util/aliases.h"
#include "../../memory/linear_allocator.h"
#include "../../memory/arena_allocator.h"
#include "../../util/renderable.h"
#include "../platform.h"

#include "../core/level/tiles/tile_manager.h"
#include "../core/level/model/voxel.h"


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
        ChunkRenderer(arena_allocator::ArenaAllocator *);

        // renderable
        auto init_shader() -> void;
        auto prepare_frame(state::State &state) -> void;
        auto frame(state::State &state) -> void;

        // write traversed voxels
        auto request_writeable_area(u64, u64) -> const VERTEX *;
        auto add_size_writeable_area(u64, u64) -> void;
        auto operator[](size_t) -> model::voxel::CubeStructure &;

        // TODO: move to renderable, let inherting classes add_textures
        // TODO: add tile manager and remove the cubestructure array
        auto add_texture(u32) -> void;

    private:
        std::vector<std::vector<Buffer<VERTEX>>> storage;
        std::array<model::voxel::CubeStructure, 512> meshes;
        linear_allocator::LinearAllocator<arena_allocator::ArenaAllocator> allocator;
        u32 texture;
    };
}


#endif //OPENGL_3D_ENGINE_CHUNK_RENDERER_H
