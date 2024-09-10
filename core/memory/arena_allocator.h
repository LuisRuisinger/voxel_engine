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

    class ArenaAllocator {
    public:
        using value_type = u8;

        ArenaAllocator();
        ~ArenaAllocator();

        explicit ArenaAllocator(const ArenaAllocator &other) =delete;
        auto operator=(const ArenaAllocator &other) -> ArenaAllocator & =delete;

        explicit ArenaAllocator(ArenaAllocator &&other) =delete;
        auto operator=(ArenaAllocator &&other) noexcept -> ArenaAllocator & =delete;

        auto destroy() -> void;
        auto deallocate(const Byte *ptr, [[maybe_unused]] const size_t len) -> void;
        auto allocate(size_t size) -> Result<Byte *, memory::Error>;

    private:
        std::atomic<memory::SLL<Byte> *> list;
    };
}

#endif //OPENGL_3D_ENGINE_ARENA_ALLOCATOR_H
