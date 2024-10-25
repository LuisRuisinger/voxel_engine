//
// Created by Luis Ruisinger on 01.09.24.
//

#ifndef OPENGL_3D_ENGINE_TIME_H
#define OPENGL_3D_ENGINE_TIME_H

#include "defines.h"
#include "traits.h"

namespace util::time {
    struct Time : public traits::Updateable<Time>  {};
}


#endif //OPENGL_3D_ENGINE_TIME_H
