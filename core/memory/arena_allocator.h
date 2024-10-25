//
// Created by Luis Ruisinger on 01.08.24.
//

#ifndef OPENGL_3D_ENGINE_ARENA_ALLOCATOR_H
#define OPENGL_3D_ENGINE_ARENA_ALLOCATOR_H

#include <thread>
#include <sys/mman.h>

#include "memory.h"
#include "../../util/result.h"

namespace core::memory::arena_allocator {
    using Byte = u8;

    template <typename T>
    struct SLL {
        size_t size;
        T *ptr;
        std::atomic<bool> used;
        std::atomic<SLL<T> *> next;
    };

    class ArenaAllocator {
    public:
        ArenaAllocator();
        ~ArenaAllocator();

        explicit ArenaAllocator(const ArenaAllocator &other) =delete;
        auto operator=(const ArenaAllocator &other) -> ArenaAllocator & =delete;

        explicit ArenaAllocator(ArenaAllocator &&other) =delete;
        auto operator=(ArenaAllocator &&other) noexcept -> ArenaAllocator & =delete;

        auto destroy() -> void;
        auto deallocate(const Byte *ptr, [[maybe_unused]] const usize len) -> void;
        auto allocate(size_t size) -> Byte *;

    private:
        auto construct_page(usize size) -> SLL<u8> *;
        auto reuse_pages(SLL<Byte> *, usize) -> Byte *;

        std::atomic<SLL<Byte> *> list;
    };
}

#endif //OPENGL_3D_ENGINE_ARENA_ALLOCATOR_H
