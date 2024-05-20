//
// Created by Luis Ruisinger on 19.03.24.
//

#ifndef OPENGL_3D_ENGINE_BOUNDINGVOLUME_H
#define OPENGL_3D_ENGINE_BOUNDINGVOLUME_H

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define LEFT_BIT   (static_cast<u64>(0x1)  << 55)
#define RIGHT_BIT  (static_cast<u64>(0x1)  << 54)
#define TOP_BIT    (static_cast<u64>(0x1)  << 53)
#define BOTTOM_BIT (static_cast<u64>(0x1)  << 52)
#define FRONT_BIT  (static_cast<u64>(0x1)  << 51)
#define BACK_BIT   (static_cast<u64>(0x1)  << 50)
#define SET_FACES  (static_cast<u64>(0x3F) << 50)

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
