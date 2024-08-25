//
// Created by Luis Ruisinger on 19.03.24.
//

#ifndef OPENGL_3D_ENGINE_VOXEL_DATA_LAYOUT_H
#define OPENGL_3D_ENGINE_VOXEL_DATA_LAYOUT_H

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#define LEFT_BIT   (static_cast<u64>(0x1)  << 55)
#define RIGHT_BIT  (static_cast<u64>(0x1)  << 54)
#define TOP_BIT    (static_cast<u64>(0x1)  << 53)
#define BOTTOM_BIT (static_cast<u64>(0x1)  << 52)
#define FRONT_BIT  (static_cast<u64>(0x1)  << 51)
#define BACK_BIT   (static_cast<u64>(0x1)  << 50)
#define SET_FACES  (static_cast<u64>(0x3F) << 50)

#endif //OPENGL_3D_ENGINE_VOXEL_DATA_LAYOUT_H
