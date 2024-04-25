//
// Created by Luis Ruisinger on 25.04.24.
//

#ifndef OPENGL_3D_ENGINE_TICKABLE_H
#define OPENGL_3D_ENGINE_TICKABLE_H

namespace util {
    class Tickable {
    public:
        virtual auto tick() -> void = 0;
    };
}

#endif //OPENGL_3D_ENGINE_TICKABLE_H
