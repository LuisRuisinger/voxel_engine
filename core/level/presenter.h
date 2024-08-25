//
// Created by Luis Ruisinger on 03.06.24.
//

#ifndef OPENGL_3D_ENGINE_PRESENTER_H
#define OPENGL_3D_ENGINE_PRESENTER_H

#include "platform.h"
#include "../../util/aliases.h"
#include "../rendering/renderer.h"
#include "../memory/linear_allocator_threadsafe.h"
#include "../memory/arena_allocator.h"
#include "model/voxel.h"
#include "model/mesh.h"

namespace core::level::presenter {

    template <typename T>
    struct Buffer {
        const T *mem;
        u64 capacity;
        u64 size;
    };

    class Presenter {
    public:
        Presenter(rendering::renderer::Renderer &, core::memory::arena_allocator::ArenaAllocator *);
        ~Presenter() =default;

        auto frame(threading::task_system::Tasksystem<> &, util::camera::Camera &) -> void;
        auto tick(threading::task_system::Tasksystem<> &, util::camera::Camera &) -> void;
        auto add_voxel_vector(std::vector<VERTEX> &&vec) -> void;
        auto get_structure(u16 i) const -> const model::voxel::CubeStructure &;

        auto request_writeable_area(u64, u64) -> const VERTEX *;
        auto add_size_writeable_area(u64, u64) -> void;

    private:
        platform::Platform platform;
        rendering::renderer::Renderer &renderer;

        std::vector<std::vector<Buffer<VERTEX>>> storage;
        std::mutex mutex;
        std::array<model::voxel::CubeStructure, 512> meshes;
        memory::linear_allocator::BumpAllocator<memory::arena_allocator::ArenaAllocator> allocator;
    };
}

#endif //OPENGL_3D_ENGINE_PRESENTER_H
