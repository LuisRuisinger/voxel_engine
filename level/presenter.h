//
// Created by Luis Ruisinger on 03.06.24.
//

#ifndef OPENGL_3D_ENGINE_PRESENTER_H
#define OPENGL_3D_ENGINE_PRESENTER_H

#include "platform.h"
#include "../util/aliases.h"
#include "../rendering/renderer.h"
#include "../util/observer.h"

namespace core::level::presenter {
    class Presenter : public util::observer::Observer {
    public:
        Presenter(rendering::Renderer &);
        ~Presenter() override = default;

        auto frame(threading::Tasksystem<> &, camera::perspective::Camera &) -> void;
        auto tick(threading::Tasksystem<> &, camera::perspective::Camera &) -> void override;
        auto update(threading::Tasksystem<> &) -> void;
        auto add_voxel_vector(std::vector<VERTEX> &&vec) -> void;
        auto get_structure(u16 i) -> VoxelStructure::CubeStructure &;

    private:
        Platform                                      _platform;
        rendering::Renderer                          &_renderer;
        std::vector<VERTEX>                           _vertices;
        std::mutex                                    _mutex;
        std::array<VoxelStructure::CubeStructure, 1>  _structures;
    };
}

#endif //OPENGL_3D_ENGINE_PRESENTER_H
