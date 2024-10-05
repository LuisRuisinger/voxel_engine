//
// Created by Luis Ruisinger on 04.09.24.
//

#ifndef OPENGL_3D_ENGINE_FRAMEBUFFER_H
#define OPENGL_3D_ENGINE_FRAMEBUFFER_H

#include "../util/aliases.h"
#include "../util/result.h"

namespace core::rendering::framebuffer {
    class Framebuffer {
        using InitFunc = std::function<void(Framebuffer &, i32, i32)>;
        using DeleteFunc = std::function<void(Framebuffer &)>;
    public:
        Framebuffer();
        Framebuffer(const Framebuffer &) =delete;
        Framebuffer(Framebuffer &&);

        ~Framebuffer();

        auto operator=(const Framebuffer &) -> Framebuffer & =delete;
        auto operator=(Framebuffer &&) -> Framebuffer &;
        auto operator=(std::pair<InitFunc, DeleteFunc>) -> Framebuffer &;

        auto bind() -> void;
        auto unbind() -> void;
        auto clear(u32) -> void;
        auto complete() -> bool;
        auto resize(i32, i32) -> void;
        auto blit(u32) -> void;

        auto get_width() const -> i32;
        auto get_height() const -> i32;

        std::vector<u32> buffer;

    private:
        auto init() -> void;
        auto destroy() -> void;

        InitFunc init_fun;
        DeleteFunc delete_fun;
        u32 fbo;

        i32 width;
        i32 height;
    };
}


#endif //OPENGL_3D_ENGINE_FRAMEBUFFER_H
