//
// Created by Luis Ruisinger on 04.09.24.
//

#include "framebuffer.h"

#include "../core/opengl/opengl_window.h"
#include "../util/assert.h"

namespace core::rendering::framebuffer {
    Framebuffer::Framebuffer()
        : width  { DEFAULT_WIDTH  },
          height { DEFAULT_HEIGHT }
    {}

    Framebuffer::Framebuffer(Framebuffer &&other)
        : buffer   { std::move(other.buffer)   },
          fbo      { other.fbo                 },
          width    { DEFAULT_WIDTH             },
          height   { DEFAULT_HEIGHT            }
    {
        other.fbo = 0;
    }

    Framebuffer::~Framebuffer() {
        unbind();
        destroy();

        glDeleteFramebuffers(1, &this->fbo);
    }

    auto Framebuffer::operator=(Framebuffer &&other) -> Framebuffer & {
        unbind();
        destroy();

        this->init_fun = std::move(other.init_fun);
        this->buffer = std::move(other.buffer);
        this->fbo = other.fbo;

        this->width = other.width;
        this->height = other.height;

        other.fbo = 0;
        return  *this;
    }

    auto Framebuffer::operator=(std::pair<InitFunc, DeleteFunc> func) -> Framebuffer & {
        if (this->init_fun) {
            unbind();
            destroy();
        }

        // its safer to destroy the framebuffer and construct a new one
        // e.g. when chanaging the size of the buffer (resizing viewportr)
        // rather than trying to modify the underlying buffers
        this->init_fun = std::move(func.first);
        this->delete_fun = std::move(func.second);
        init();

        return *this;
    }

    auto Framebuffer::complete() -> bool {
        return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    }

    auto Framebuffer::resize(i32 width, i32 height) -> void {
        destroy();

        this->width = width;
        this->height = height;
        init();
    }

    auto Framebuffer::bind() -> void {
        glBindFramebuffer(GL_FRAMEBUFFER, this->fbo);
    }

    auto Framebuffer::unbind() -> void {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    auto Framebuffer::clear(u32 mask) -> void {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(mask);
    }

    auto Framebuffer::init() -> void {
        glGenFramebuffers(1, &this->fbo);
        bind();
        this->init_fun(*this, this->width, this->height);

        ASSERT_EQ(complete());
        unbind();
    }

    auto Framebuffer::destroy() -> void {
        this->delete_fun(*this);
    }

    auto Framebuffer::blit(u32 target) -> void {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, this->fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target);
        glBlitFramebuffer(
                0, 0, this->width, this->height,
                0, 0, this->width, this->height,
                GL_DEPTH_BUFFER_BIT,
                GL_NEAREST
        );

        // the target framebuffer is now the active framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, target);
    }

    auto Framebuffer::get_width() const -> i32 {
        return this->width;
    }

    auto Framebuffer::get_height() const -> i32 {
        return this->height;
    }
}