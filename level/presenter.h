//
// Created by Luis Ruisinger on 03.06.24.
//

#ifndef OPENGL_3D_ENGINE_PRESENTER_H
#define OPENGL_3D_ENGINE_PRESENTER_H

#include "platform.h"
#include "../util/aliases.h"
#include "../rendering/renderer.h"
#include "../util/observer.h"
#include "../core/memory/linear_allocator_threadsafe.h"
#include "../core/memory/arena_allocator.h"

namespace core::level::presenter {

    template <typename T>
    struct Buffer {
        const T *mem;
        u64 capacity;
        u64 size;
    };

    class Presenter : public util::observer::Observer {
    public:
        Presenter(rendering::Renderer &, core::memory::arena_allocator::ArenaAllocator *);
        ~Presenter() override = default;

        auto frame(threading::Tasksystem<> &, camera::perspective::Camera &) -> void;
        auto tick(threading::Tasksystem<> &, camera::perspective::Camera &) -> void override;
        auto update(threading::Tasksystem<> &) -> void;
        auto add_voxel_vector(std::vector<VERTEX> &&vec) -> void;
        auto get_structure(u16 i) const -> const VoxelStructure::CubeStructure &;

        auto request_writeable_area(u64, u64) -> const VERTEX *;
        auto add_size_writeable_area(u64, u64) -> void;

    private:
        Platform platform;
        rendering::Renderer &renderer;

        std::vector<std::vector<Buffer<VERTEX>>> storage;
        std::mutex mutex;
        std::array<VoxelStructure::CubeStructure, 1> meshes;
        memory::linear_allocator::BumpAllocator<memory::arena_allocator::ArenaAllocator> allocator;
    };
}

#endif //OPENGL_3D_ENGINE_PRESENTER_H
