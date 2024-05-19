//
// Created by Luis Ruisinger on 14.03.24.
//

#ifndef OPENGL_3D_ENGINE_VOXEL_H
#define OPENGL_3D_ENGINE_VOXEL_H


#include "../Mesh.h"

namespace VoxelStructure {
    class Voxel {
        virtual auto mesh() const -> const std::array<std::vector<u64>, 6> & =0;
    };

    class CubeStructure : Voxel {
    public:
        CubeStructure();
        ~CubeStructure() = default;
        auto mesh() const -> const std::array<std::vector<u64>, 6> &;
        auto setFace(std::vector<Mesh::Vertex> &face, u8 idx) -> void;

    private:
        std::array<std::vector<u64>, 6> _compressedFaces;
    };
}



#endif //OPENGL_3D_ENGINE_VOXEL_H
