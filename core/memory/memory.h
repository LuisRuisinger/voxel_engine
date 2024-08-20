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

#include "../util/aliases.h"
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
        INVALID_PAGE_SIZE
    };

    inline auto calculate_padding(const u8 *addr, u64 align) -> u64 {
        u64 addr_missalignment = reinterpret_cast<u64>(addr) % align;
        u64 addr_padding = addr_missalignment
                ? (align - addr_missalignment)
                : 0;

        ASSERT_NEQ(
                reinterpret_cast<u64>(addr + addr_padding) % align,
                "address is not aligned");
        return addr_padding;
    }
}

#endif //OPENGL_3D_ENGINE_MEMORY_H
