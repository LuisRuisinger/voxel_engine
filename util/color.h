//
// Created by Luis Ruisinger on 30.09.24.
//

#ifndef OPENGL_3D_ENGINE_COLOR_H
#define OPENGL_3D_ENGINE_COLOR_H

#include <array>

#include "../core/opengl/opengl_verify.h"
#include "aliases.h"

namespace util {
    namespace color {
        constexpr auto u32_to_rgba(u32 color) -> glm::vec4 {
            return {
                static_cast<f32>((color >> 24) & 0xFF) / 255.0F,
                static_cast<f32>((color >> 16) & 0xFF) / 255.0F,
                static_cast<f32>((color >>  8) & 0xFF) / 255.0F,
                static_cast<f32>(color & 0xFF)         / 255.0F
            };
        }

        constexpr auto u32_to_rgb(u32 color) -> glm::vec4 {
            return {
                static_cast<f32>((color >> 24) & 0xFF) / 255.0F,
                static_cast<f32>((color >> 16) & 0xFF) / 255.0F,
                static_cast<f32>((color >>  8) & 0xFF) / 255.0F,
                1.0F
            };
        }

        template <typename ...Args>
        constexpr auto u32_to_rgba_texture(Args ...args) -> GLuint {
            std::array<
                typename std::decay<typename std::common_type<Args ...>::type>::type,
                sizeof...(Args)> arr = { std::forward<Args>(args)... };

            GLuint texture;
            OPENGL_VERIFY(glGenTextures(1, &texture));
            OPENGL_VERIFY(glBindTexture(GL_TEXTURE_1D, texture));

            // Set texture parameters
            OPENGL_VERIFY(glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
            OPENGL_VERIFY(glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
            OPENGL_VERIFY(glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
            OPENGL_VERIFY(glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

            std::cout << arr.size() ;


            OPENGL_VERIFY(glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, arr.size(), 0, GL_RGBA32UI, GL_UNSIGNED_BYTE, arr.data()));
            OPENGL_VERIFY(glBindTexture(GL_TEXTURE_1D, 0));
            return texture;
        }

        template <typename ...Args>
        constexpr auto u32_to_rgb_texture(Args ...args) -> GLuint {
            return u32_to_rgba_texture(std::forward<Args>(args | 0xFF)...);
        }
    }
}

#endif //OPENGL_3D_ENGINE_COLOR_H
