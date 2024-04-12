//
// Created by Luis Ruisinger on 06.03.24.
//

#ifndef OPENGL_3D_ENGINE_RENDERER_H
#define OPENGL_3D_ENGINE_RENDERER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../global.h"
#include "../Shader.h"
#include "../Level/Model/Mesh.h"
#include "../camera.h"
#include "../Level/Model/Types/Voxel.h"
#include "../Level/Octree/BoundingVolume.h"

#define MAX_VERTICES_BUFFER ((u32) (131072 * 2))
#define INDICES_PER_FACE 6

namespace Renderer {
    struct Vertex {
        vec3f pos;
    };

    template<u32 N = MAX_VERTICES_BUFFER>
    struct IndicesGenerator {
        IndicesGenerator() : arr() {
            u32 genIdx = 0;

            for (u32 i = 0; i < (N * INDICES_PER_FACE) - INDICES_PER_FACE; i += INDICES_PER_FACE) {
                arr[i] = genIdx;
                arr[i + 1] = genIdx + 1;
                arr[i + 2] = genIdx + 3;
                arr[i + 3] = genIdx + 1;
                arr[i + 4] = genIdx + 2;
                arr[i + 5] = genIdx + 3;

                genIdx += 4;
            }
        }

        constexpr auto end() -> u32 * {
            return &this->arr[N * INDICES_PER_FACE];
        }

        u32 arr[N * INDICES_PER_FACE];
    };

    class Renderer {
    public:
        explicit Renderer(std::shared_ptr<Camera::Camera> camera);

        ~Renderer() = default;

        auto initGLFW() -> void;

        auto initShaders() -> void;

        auto initPipeline() -> void;

        auto addVoxel(const BoundingVolume *) const -> void;

        auto updateBuffer() -> void;

        auto updateProjectionMatrix() -> void;

        auto draw(u32 texture) -> void;

        auto getCamera() const -> const Camera::Camera *;

        auto getWindow() const -> const GLFWwindow *;

    private:

        //
        //

        u32 width;
        u32 height;
        GLFWwindow *window;

        //
        //

        glm::mat4 projection;
        std::unique_ptr<Shader> shader;

        // ------------------------------------------------------------------------------------
        // dynamic vertex vector - contains the current visible verticies for the hooked camera

        std::unique_ptr<std::vector<Vertex>> mutable vertices;

        // ---------------------
        // static indices vector

        std::unique_ptr<std::vector<u32>> indices;

        // ------------------------------------------------------------------
        // currently hooked camera, can be owned an used by multiple classes,
        // e.g. player hook, renderer hook

        std::shared_ptr<Camera::Camera> camera;

        // -----------
        // GPU buffers

        u32 VBO;
        u32 VAO;
        u32 EBO;

        // ---------------
        // cube structures

        std::array<VoxelStructure::CubeStructure, 1> structures;
    };
}


#endif //OPENGL_3D_ENGINE_RENDERER_H
