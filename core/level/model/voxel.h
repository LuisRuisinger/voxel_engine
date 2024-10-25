//
// Created by Luis Ruisinger on 14.03.24.
//

#ifndef OPENGL_3D_ENGINE_VOXEL_H
#define OPENGL_3D_ENGINE_VOXEL_H

#include <vector>

#include "mesh.h"
#include "../util/defines.h"

namespace core::level::model::voxel {
    class Voxel {
    public:
#ifdef __AVX2__
        using Compressed = std::array<__m256i, 6>;
#else
        using Compressed = std::array<std::vector<u64>, 6>;
#endif
        constexpr virtual auto mesh() const -> const Compressed & =0;
    };

    class CubeStructure : public Voxel {
    public:
        CubeStructure();
        auto mesh() const -> const Compressed &;
        auto compress_face(std::vector<Mesh::Vertex> &, u8) -> void;

    private:
        Compressed compressed_faces;
    };

    /** @brief Object containing a compressed representation of the sides of a voxel. */
    static const CubeStructure cube_structure = {};
}

#endif //OPENGL_3D_ENGINE_VOXEL_H
