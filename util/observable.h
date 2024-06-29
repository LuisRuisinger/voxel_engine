//
// Created by Luis Ruisinger on 04.06.24.
//

#ifndef OPENGL_3D_ENGINE_OBSERVABLE_H
#define OPENGL_3D_ENGINE_OBSERVABLE_H

#include "observer.h"

namespace util::observable {
    class Observable {
    public:
        virtual ~Observable() {}

        auto virtual attach(observer::Observer *) -> void =0;
        auto virtual detach(observer::Observer *) -> void =0;
        auto virtual notfiy() -> void =0;
    };
}

#endif //OPENGL_3D_ENGINE_OBSERVABLE_H
