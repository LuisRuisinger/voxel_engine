//
// Created by Luis Ruisinger on 25.04.24.
//

#ifndef OPENGL_3D_ENGINE_UPDATEABLE_H
#define OPENGL_3D_ENGINE_UPDATEABLE_H

#endif //OPENGL_3D_ENGINE_UPDATEABLE_H

namespace util {
    class Updateable {
    public:
        virtual auto update() -> void = 0;
    };
}