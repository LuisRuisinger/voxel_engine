//
// Created by Luis Ruisinger on 24.08.24.
//

#ifndef OPENGL_3D_ENGINE_OPENGL_VERIFY_H
#define OPENGL_3D_ENGINE_OPENGL_VERIFY_H

#include "../../util/log.h"
#include "GLFW/glfw3.h"

namespace core::opengl::opengl_verify {
    enum OpenGLError : i32 {
        INVALID_ENUM                  = GL_INVALID_ENUM,
        INVALID_VALUE                 = GL_INVALID_VALUE,
        INVALID_OPERATION             = GL_INVALID_OPERATION,
        // STACK_OVERFLOW                = GL_STACK_OVERFLOW,
        // STACK_UNDERFLOW               = GL_STACK_UNDERFLOW,
        OUT_OF_MEMORY                 = GL_OUT_OF_MEMORY,
        INVALID_FRAMEBUFFER_OPERATION = GL_INVALID_FRAMEBUFFER_OPERATION,
    };
}

#ifdef DEBUG
    #define OPENGL_VERIFY(_x) do {                           \
        (_x);                                                \
        DEBUG_LOG(#_x);                                      \
        while ((GLenum error = glGetError()) != GL_NO_ERROR) \
            DEBUG_LOG(error);                                \
    } while (0)
#else
    #define OPENGL_VERIFY(_x) (_x)
#endif

#endif //OPENGL_3D_ENGINE_OPENGL_VERIFY_H
