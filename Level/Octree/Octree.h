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

#include "../../global.h"
#include "glad/glad.h"
#include "../Model/Mesh.h"
#include "../../Rendering/Renderer.h"

#define BASE {CHUNK_SIZE / 2, CHUNK_SIZE / 2, CHUNK_SIZE / 2}
#define CHUNK_BVEC {CHUNK_SIZE, BASE}
#define BASE_SIZE 1
#define ZERO_FACES(x) (x & ~(0b111111 << 10))
#define EXTR_FACES(x) ((x >> 10) & 0b111111)
#define RENDER_RADIUS 3
#define RENDER_DISTANCE (RENDER_RADIUS * CHUNK_SIZE)

namespace Octree {
    enum ChunkData {
        DATA, NODATA
    };

    //
    //
    //

    template<typename T>
    struct Args {
        const vec3f              &_point;
        const Camera::Camera     &_camera;
        const Renderer::Renderer &_renderer;
    };

    //
    //
    //

    template <typename T>
    concept derivedFromBoundingVolume = requires(T t) {
        std::is_base_of<BoundingVolume, T>::value;
    };

    //
    //
    //


    template<typename T> requires derivedFromBoundingVolume<T>
    struct Node {
        Node();
        ~Node() noexcept;

        auto insert(vec3f, T t, std::pair<f32, vec3f> currentBVol) -> Node<T> *;
        auto removePoint(glm::vec3, std::pair<f32, glm::vec3>) -> void;
        auto cull(const Args<T> &) const -> void;
        auto find(vec3f, std::pair<f32, vec3f>) const -> std::optional<Node<T> *>;
        auto updateFaceMask(const std::pair<f32, vec3f>&) -> u8;
        auto recombine(std::stack<Node *> &stack) -> void;

        // ----------------------------------------------
        // either points to a bounding box or child nodes

        union {
            BoundingVolume *_leaf;
            Node           *_nodes;
        };

        // ------------------------------------------------------
        // every bit represents which child node segment is empty
        // 0 bit - empty
        // in the case every bit is empty but the pointer is not null
        // we know the stored pointer points to a bouding box

        u8 _segments;
        u8 _faces;

        std::pair<u32, vec3f> _boundingVolume;
    };

    //
    //
    //

    template<typename T> requires derivedFromBoundingVolume<T>
    class Octree {
    public:
        explicit Octree(vec3f);
        ~Octree() = default;

        auto addPoint(vec3f position, T t) -> Node<T> *;
        auto removePoint(vec3f point) -> void;
        auto cull(const vec3f &, const Camera::Camera &, const Renderer::Renderer&) const -> void;
        auto find(const vec3f &) const -> std::optional<Node<T> *>;
        auto updateFaceMask() -> u8;
        auto recombine() -> void;

    private:
        std::unique_ptr<Node<T>>  _root;

        // ------------------------------------------------------------
        // the biggest possible volume bounding the space of the octree

        const std::pair<u16, vec3f> _boundingVolume;
    };
}

#endif //OPENGL_3D_ENGINE_OCTREE_H
