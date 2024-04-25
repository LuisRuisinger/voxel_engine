//
// Created by Luis Ruisinger on 14.03.24.
//

#ifndef OPENGL_3D_ENGINE_VOXEL_H
#define OPENGL_3D_ENGINE_VOXEL_H


#include "../Mesh.h"

namespace VoxelStructure {
    class Voxel {
        virtual auto structure() const -> const std::vector<Mesh::Mesh>& = 0;
    };

    class CubeStructure : Voxel {
    public:
        CubeStructure();
        ~CubeStructure() = default;
        auto structure() const -> const std::vector<Mesh::Mesh>&;

    private:
        std::vector<Mesh::Mesh> faces;

    };
}



#endif //OPENGL_3D_ENGINE_VOXEL_H
