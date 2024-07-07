//
// Created by Luis Ruisinger on 03.06.24.
//

#include "presenter.h"

namespace core::level::presenter {
    Presenter::Presenter(rendering::Renderer &renderer)
        : util::observer::Observer{}
        , _structures{}
        , _platform{*this}
        , _renderer{renderer}
    {
        this->_structures[0] = VoxelStructure::CubeStructure{};
    }

    auto Presenter::frame(threading::Tasksystem<> &thread_pool, camera::perspective::Camera &camera) -> void {
        auto t_start = std::chrono::high_resolution_clock::now();
        this->_platform.frame(thread_pool, camera);
        auto t_end  = std::chrono::high_resolution_clock::now();
        auto t_diff = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start);

        this->_renderer.updateGlobalBase(this->_platform.get_world_root());
        this->_renderer.updateRenderDistance(RENDER_RADIUS);

        for (size_t i = 0; i < this->_vertices.size(); i += this->_renderer.get_batch_size()) {
            this->_renderer.updateBuffer(
                    this->_vertices.data() + i,
                    fmin(this->_vertices.size() - i, this->_renderer.get_batch_size()));
            this->_renderer.frame();
        }

        rendering::interface::set_render_time(t_diff);
        rendering::interface::set_vertices_count(this->_vertices.size() * sizeof(VERTEX) / sizeof(u64));
        rendering::interface::set_camera_pos(camera.getCameraPosition());
        rendering::interface::update();
        rendering::interface::render();

        this->_vertices.clear();
    }

    auto Presenter::tick(threading::Tasksystem<> &thread_pool, camera::perspective::Camera &camera) -> void  {
        this->_platform.tick(thread_pool, camera);
    }

    auto Presenter::add_voxel_vector(std::vector<VERTEX> &&vec) -> void {
        {
            std::scoped_lock lock{this->_mutex};
            this->_vertices.insert(this->_vertices.end(), vec.begin(), vec.end());
        }
    }

    auto Presenter::get_structure(u16 i) -> VoxelStructure::CubeStructure & {
        return this->_structures[i];
    }
}