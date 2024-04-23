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
#define SET_FACES  (0x3F << 10)

struct BoundingVolume {

    // ---------------------------------------------
    // the size and scale is known by the owner node
    // 6 highest bit represent occlusion of faces

    u16  _voxelID;
};

struct BoundingVolumeVoxel : BoundingVolume {
    vec3f _position;
    u32   _scale;

    BoundingVolumeVoxel(u16 voxelID, vec3f position, u32 scale)
        : BoundingVolume{voxelID}
        , _position{position}
        , _scale{scale}
    {}
};


#endif //OPENGL_3D_ENGINE_BOUNDINGVOLUME_H
