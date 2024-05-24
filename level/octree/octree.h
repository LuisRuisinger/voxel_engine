//
// Created by Luis Ruisinger on 17.02.24.
//

#ifndef OPENGL_3D_ENGINE_OCTREE_H
#define OPENGL_3D_ENGINE_OCTREE_H

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <memory>
#include <array>
#include <vector>
#include <stack>
#include <functional>

#include "../../util/aliases.h"
#include "glad/glad.h"
#include "../Model/mesh.h"
#include "../../Rendering/renderer.h"

#define BASE {CHUNK_SIZE / 2, CHUNK_SIZE / 2, CHUNK_SIZE / 2}
#define CHUNK_BVEC {CHUNK_SIZE, BASE}
#define BASE_SIZE 1

#define EXTRACT_SCALE(_ptr)        (((_ptr)->_packed >> 50) & 0x3F)
#define EXTRACT_SEGMENTS(_ptr)     ((_ptr)->_packed >> 56)
#define EXTRACT_

#define SET_SCALE(_ptr, _value)       ((_ptr)->_packed = ((_ptr)->_packed & ~(0x3F << 50)) | (((_values) & 0x3F) << 50))
#define SET_SEGMENTS(_ptr, _value)    ((_ptr)->_packed = ((_ptr)->_packed & ~(0xFF << 56)) | (((_values) & 0xFF) << 56))
#define SHIFT_X 45
#define SHIFT_Y 40
#define SHIFT_Z 35


namespace core::level::octree {
    struct Args {
        const glm::vec3                   &_point;
        const camera::perspective::Camera &_camera;
        const rendering::Renderer         &_renderer;
        std::vector<VERTEX>               &_voxelVec;
    };

    //
    //
    //


    struct Node {
        Node();
        ~Node() noexcept;

        auto cull(const Args &) const -> void;
        auto updateFaceMask(u16) -> u8;
        auto recombine(std::stack<Node *> &stack) -> void;

        // segments 0 indicate either not in use or voxel
        // in case of not in use or voxel not visible the faces will be set to 0
        // we check existance of a voxel via an ID set to anything else than 0 (lowest 8 bit)
        Node *_nodes;
        u64   _packed;




        // segments: 8 | faces: 6 | curX: 5 | curY: 5 | curZ: 5 | scale: 3 (exactly 32)
        // chunkIndex2D: 12 | chunkSegmentOffsetY: 4 | unused: 8 | voxelID: 8 (exactly 32)

        // unused: 14 | curX: 5 | curY: 5 | curZ: 5 | scale: 3 (exactly 32)
        // chunkIndex2D: 12 | chunkSegmentOffsetY: 4 | unused: 8 | voxelID: 8 (exactly 32)

        // offsetX: 3 | offsetY: 3 | offsetZ: 3 | uv: 4 | unused: 1 | curX: 5 | curY: 5 | curZ: 5 | scale: 3 (exactly 32)
        // chunkIndex2D: 12 | chunkSegmentOffsetY: 4 | normals: 3 | unused: 5 | voxelID: 8 (exactly 32)
    };

    //
    //
    //

    class Octree {
    public:
        Octree();
        ~Octree() = default;

        auto addPoint(u64) -> Node *;
        auto removePoint(u16) -> void;
        auto cull(const glm::vec3 &position,
                  const camera::perspective::Camera &camera,
                  const rendering::Renderer &renderer,
                  std::vector<VERTEX> &voxelVec) const -> void;
        auto find(u32) const -> std::optional<Node *>;
        auto updateFaceMask(u16) -> u8;
        auto recombine() -> void;

    private:
        std::unique_ptr<Node>  _root;

        // sets the base bounding volume for an octree
        const u32 _packed = (0x3F << 18) | (0x10 << 13) | (0x10 << 8) | (0x10 << 3) | 5;

    };
}

#endif //OPENGL_3D_ENGINE_OCTREE_H