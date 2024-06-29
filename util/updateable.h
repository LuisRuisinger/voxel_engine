//
// Created by Luis Ruisinger on 25.04.24.
//

#ifndef OPENGL_3D_ENGINE_UPDATEABLE_H
#define OPENGL_3D_ENGINE_UPDATEABLE_H

#endif //OPENGL_3D_ENGINE_UPDATEABLE_H

#include "../threading/thread_pool.h"

namespace util {
    class Updateable {
    public:
        virtual auto update(core::threading::Tasksystem<> &) -> void = 0;
    };
}