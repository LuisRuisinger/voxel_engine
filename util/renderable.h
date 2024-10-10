//
// Created by Luis Ruisinger on 26.08.24.
//

#ifndef OPENGL_3D_ENGINE_RENDERABLE_H
#define OPENGL_3D_ENGINE_RENDERABLE_H

#include "../core/state.h"
#include "../core/rendering/shader.h"
#include "../core/opengl/opengl_verify.h"

#include "indices_generator.h"
#include "aliases.h"

namespace util::renderable {
    using namespace core::rendering;

    enum AttribType {
        FLOAT_CONV,
        INT_CONV,
        DOUBLE_PREC
    };

    enum Type : u32 {
        BYTE    = GL_BYTE,
        U_BYTE  = GL_UNSIGNED_BYTE,
        SHORT   = GL_SHORT,
        U_SHORT = GL_UNSIGNED_SHORT,
        INT     = GL_INT,
        U_INT   = GL_UNSIGNED_INT,
        FLOAT   = GL_FLOAT,
        H_FLOAT = GL_HALF_FLOAT,
        DOUBLE  = GL_DOUBLE
    };

    struct BaseInterface {
        virtual ~BaseInterface() {}

        virtual auto init() -> void =0;
        virtual auto prepare_frame(core::state::State &) -> void =0;
        virtual auto frame(core::state::State &) -> void =0;
        virtual auto draw() -> void =0;
    };

    template <typename T>
    class Renderable : public BaseInterface {
    public:

        Renderable() : buffer_offset { 0 }, buffer_handle { nullptr } {}

        auto _crtp_init() -> void {
            static_cast<T *>(this)->init();
        }

        auto _crtp_prepare_frame(core::state::State &state) -> void {
            static_cast<T *>(this)->prepare_frame(state);
        }

        auto _crtp_frame(core::state::State &state) -> void {
            glBindVertexArray(this->layout.VAO);
            static_cast<T *>(this)->frame(state);
        }

        inline constexpr auto batch(size_t align) const -> size_t {
            return MAX_VERTICES_BUFFER * sizeof(u64) / align;
        }

        auto draw() -> void {
            drop_buffer();

            glDrawElements(
                    GL_TRIANGLES,
                    static_cast<u32>(this->vertex_count * 1.5F),
                    GL_UNSIGNED_INT,
                    nullptr);

            this->vertex_count = 0;
        }

        auto update_buffer(const void *ptr, size_t align, size_t len) -> void {
            if (!this->buffer_handle)
                get_buffer();

            std::memcpy(this->buffer_handle + this->buffer_offset, ptr, len * align);
            this->vertex_count +=
                    static_cast<size_t>(
                            static_cast<f32>(len * align) /
                            static_cast<f32>(this->layout.sz));

            this->buffer_offset += len * align;
            ASSERT_EQ(this->vertex_count);
            glUnmapBuffer(GL_ARRAY_BUFFER);
        }

    protected:
        class Layout {
            friend Renderable<T>;
        public:
            Layout()
                    : cnt { 0 } {
                auto indices_buffer = util::IndicesGenerator<MAX_VERTICES_BUFFER>();
                this->indices.insert(
                        this->indices.end(),
                        indices_buffer.arr,
                        indices_buffer.end());
            }

            auto begin(size_t size) -> Layout & {
                this->sz = size;
                ASSERT_EQ(this->sz);

                // VAO generation
                glGenVertexArrays(1, &this->VAO);
                glBindVertexArray(this->VAO);

                // buffer generation
                glGenBuffers(1, &this->VBO);
                glGenBuffers(1, &this->EBO);

                glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
                glBufferData(
                        GL_ARRAY_BUFFER,
                        MAX_VERTICES_BUFFER * sizeof(u64),
                        nullptr,
                        GL_DYNAMIC_DRAW);

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
                glBufferData(
                        GL_ELEMENT_ARRAY_BUFFER,
                        this->indices.size() * sizeof(u32),
                        this->indices.data(),
                        GL_STATIC_DRAW);

                return *this;
            }

            auto add(
                    u32 amount,
                    Type rtype,
                    GLvoid *offset,
                    GLboolean normalized = false) -> Layout & {
                if (rtype != Type::FLOAT &&
                    rtype != Type::H_FLOAT &&
                    rtype != Type::DOUBLE) {

                    glVertexAttribIPointer(this->cnt, amount, rtype, this->sz, offset);
                    glEnableVertexAttribArray(this->cnt);
                    ++this->cnt;
                }
                else {
                    glVertexAttribPointer(this->cnt, amount, rtype, normalized, this->sz, offset);
                    glEnableVertexAttribArray(this->cnt);
                    ++this->cnt;
                }

                return *this;
            }

            auto add(
                    u32 amount,
                    Type rtype,
                    size_t offset,
                    GLboolean normalized = false) -> Layout & {
                return add(amount, rtype, reinterpret_cast<GLvoid *>(offset), normalized);
            }

            auto end() -> void {
                glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
            }

            inline auto size() -> size_t {
                return this->sz;
            }

        private:
            GLuint VAO;
            GLuint VBO;
            GLuint EBO;

            size_t cnt;
            size_t sz;

            std::vector<u32> indices;
        };

        Layout layout;

        u64 buffer_offset;
        u8 *buffer_handle;

    private:
        auto get_buffer() -> void {
            glBufferData(
                    GL_ARRAY_BUFFER,
                    MAX_VERTICES_BUFFER * sizeof(u64),
                    nullptr,
                    GL_DYNAMIC_DRAW);

            this->buffer_handle =
                    static_cast<u8 *>(
                            glMapBufferRange(
                                    GL_ARRAY_BUFFER,
                                    0,
                                    MAX_VERTICES_BUFFER * sizeof(u64),
                                    GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));
        }

        auto drop_buffer() -> void {
            glUnmapBuffer(GL_ARRAY_BUFFER);
            this->buffer_handle = nullptr;
            this->buffer_offset = 0;
        }

        size_t vertex_count;
    };
}

#endif //OPENGL_3D_ENGINE_RENDERABLE_H