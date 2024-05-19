//
// Created by Luis Ruisinger on 06.03.24.
//

#ifndef OPENGL_3D_ENGINE_RENDERER_H
#define OPENGL_3D_ENGINE_RENDERER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../util/aliases.h"
#include "Shader.h"
#include "../Level/Model/Mesh.h"
#include "../camera/camera.h"
#include "../Level/Model/Types/Voxel.h"
#include "../Level/Octree/BoundingVolume.h"

#define MAX_VERTICES_BUFFER ((u32) (131072 * 2))
#define MAX_RENDER_VOLUME (64 * 64)

namespace Renderer {

    //
    //
    //

    //
    //
    //

    Enum(Buffer, VAO, VBO, EBO);

    //
    //
    //

    class Renderer {
    public:
        explicit Renderer(std::shared_ptr<Camera::Perspective::Camera> camera);

        ~Renderer() = default;

        auto initGLFW() -> void;
        auto initShaders() -> void;
        auto initPipeline() -> void;

        auto addVoxel(u64) const -> void;

        auto prepareBuffer() -> void;
        auto updateBuffer(size_t) -> size_t;
        auto updateProjectionMatrix() -> void;
        auto updateGlobalBase(vec2f) -> void;
        auto updateRenderDistance(u32) -> void;

        auto frame() -> void;
        auto draw(u32) -> void;
        auto flush() -> void;

        auto getCamera() const -> const Camera::Perspective::Camera *;
        auto getWindow() const -> const GLFWwindow *;

    private:

        // general renderer data
        u32                                           _width;
        u32                                           _height;
        GLFWwindow                                   *_window;
        std::shared_ptr<Camera::Perspective::Camera>  _camera;

        // vertex shader data
        // the chunk positions are compressed into 2 * 6 bit
        glm::mat4               _projection;
        std::unique_ptr<Shader> _shader;

        // dynamic vertex vector - contains the current visible verticies for the hooked camera
        std::unique_ptr<std::vector<u64>> mutable _vertices;

        std::unique_ptr<std::vector<u32>>            _indices;

        // GPU buffers
        GLuint _buffers[Buffer::count];

        // cube structures
        std::array<VoxelStructure::CubeStructure, 1> _structures;
    };
}


#endif //OPENGL_3D_ENGINE_RENDERER_H
