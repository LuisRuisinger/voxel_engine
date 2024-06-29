//
// Created by Luis Ruisinger on 04.06.24.
//

#ifndef OPENGL_3D_ENGINE_OBSERVER_H
#define OPENGL_3D_ENGINE_OBSERVER_H

#include "../threading/thread_pool.h"
#include "../camera/camera.h"

namespace util::observer {

    class Observer {
    public:
        virtual ~Observer() {};

        auto virtual tick(core::threading::Tasksystem<> &, core::camera::perspective::Camera &) -> void =0;
    };
}

#endif //OPENGL_3D_ENGINE_OBSERVER_H
