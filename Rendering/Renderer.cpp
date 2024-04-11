//
// Created by Luis Ruisinger on 15.03.24.
//
#include <immintrin.h>

#include "Renderer.h"

namespace Renderer {
    Renderer::Renderer(std::shared_ptr<Camera::Camera> camera) {
        this->camera   = std::move(camera);
        this->vertices = std::make_unique<std::vector<Vertex>>();
        this->indices  = std::make_unique<std::vector<u32>>();

        auto indices_buffer = IndicesGenerator<MAX_VERTICES_BUFFER>();
        this->indices->insert(this->indices->end(), indices_buffer.arr, indices_buffer.end());

        this->structures[0] = VoxelStructure::CubeStructure {};

        glEnable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        glGenVertexArrays(1, &this->VAO);
        glBindVertexArray(this->VAO);

        glGenBuffers(1, &this->VBO);
        glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
        glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES_BUFFER * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

        glGenBuffers(1, &this->EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.get()->size() * sizeof(uint32_t), &(this->indices->at(0)), GL_STATIC_DRAW);

        // vertex object space position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
        glEnableVertexAttribArray(0);
    }

    auto Renderer::addVoxel(const BoundingVolume *bVol) const -> void {
        const auto &mesh = structures[bVol->voxelID & UINT8_MAX].structure();

        for (u8 i = 0; i < 6; ++i) {
            if (bVol->voxelID & (1 << (i + 10))) {
                const auto &currentMesh = mesh[i];
                const auto &verts = currentMesh.vertices;

                this->vertices->reserve(this->vertices->size() + verts.size());

                for (const auto &vert : verts) {
                    this->vertices->push_back({
                        .pos = bVol->position + vert.position * ((f32) bVol->scale),
                    });
                }
            }
        }
    }

    auto Renderer::updateBuffer() -> void {
        if (this->vertices->empty())
            return;

        glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
        void *bufferData =
                glMapBufferRange(GL_ARRAY_BUFFER,
                                 0,
                                 this->vertices->size() * sizeof(Vertex),
                                 GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

        __builtin_memcpy(bufferData, this->vertices->data(), this->vertices->size() * sizeof(Vertex));
        glUnmapBuffer(GL_ARRAY_BUFFER);
    }

    auto Renderer::draw(Shader &shader, glm::mat4 &proj_matrix, u32 texture) -> void {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        auto view          = this->camera->GetViewMatrix();
        auto viewLoc       = glGetUniformLocation(shader.ID, "view");
        auto projectionLoc = glGetUniformLocation(shader.ID, "projection");

        glUniformMatrix4fv(viewLoc,       1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(proj_matrix));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(this->VAO);
        glDrawElements(GL_TRIANGLES, static_cast<int>(this->indices->size()), GL_UNSIGNED_INT, nullptr);

        this->vertices->clear();
    }

    auto Renderer::getCamera() -> const Camera::Camera * {
        return this->camera.get();
    }
}