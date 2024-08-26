//
// Created by Luis Ruisinger on 01.08.24.
//

#ifndef OPENGL_3D_ENGINE_MEMORY_H
#define OPENGL_3D_ENGINE_MEMORY_H

#include <cstdlib>
#include <memory>
#include <optional>
#include <ranges>
#include <atomic>

#include "../../util/aliases.h"
#include "../util/assert.h"

#define KByte(_v)  ((_v) * 1000)
#define MByte(_v)  ((_v) * 1000 * 1000)
#define GByte(_v)  ((_v) * 1000 * 1000 * 1000)

#define KiByte(_v) ((_v) * 1024)
#define MiByte(_v) ((_v) * 1024 * 1024)
#define GiByte(_v) ((_v) * 1024 * 1024)

#define HUGE_PAGE  (MiByte(2))

namespace core::memory::memory {
    template <typename T>
    struct SLL {
        u64 size;
        T *ptr;
        std::atomic<bool> used;
        std::atomic<SLL<T> *> next;
    };

    enum Error : u8 {
        ALLOC_FAILED,
        ALLOC_PAGE_FAILED,
        ALLOC_MAX_SEGMENT_SIZE_EXCEEDED,
        ALLOC_MAX_BUFFER_SIZE_EXCEEDED,
        ALLOC_NO_LHPT_SUPPORT,
        ALLOC_UNKNOWN_ALLOCATOR,
        INVALID_PAGE_SIZE,
        MEMORY_GUARD_ACTIVE
    };

    inline auto ptr_offset(void *ptr, size_t align) noexcept -> void * {
        return reinterpret_cast<void *>((-reinterpret_cast<intptr_t>(ptr)) & (align - 1));
    }


}

#endif //OPENGL_3D_ENGINE_MEMORY_H
