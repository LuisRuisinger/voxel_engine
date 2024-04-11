//
// Created by Luis Ruisinger on 19.03.24.
//

#ifndef OPENGL_3D_ENGINE_BOUNDINGVOLUME_H
#define OPENGL_3D_ENGINE_BOUNDINGVOLUME_H

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define LEFT_BIT   (0x1 << 15)
#define RIGHT_BIT  (0x1 << 14)
#define TOP_BIT    (0x1 << 13)
#define BOTTOM_BIT (0x1 << 12)
#define FRONT_BIT  (0x1 << 11)
#define BACK_BIT   (0x1 << 10)

struct BoundingVolume {
    // ------------------------------------------
    // 6 highest bit represent occlusion of faces
    uint16_t  voxelID;

    // ------------------------------------
    // scale^3 occupied space by the volume

    uint8_t   scale;

    // -------------------------
    // position inside the chunk

    glm::vec3 position;

    /*
     *
     * u8 voxelID
     * u8 faces
     * u8 scale
     * u8 free
     *
     * -> positions are being calculated via the adapter
     */
};


#endif //OPENGL_3D_ENGINE_BOUNDINGVOLUME_H
