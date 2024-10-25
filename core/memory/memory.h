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

#include "../util/defines.h"
#include "../util/assert.h"

#define KByte(_v)  ((_v) * 1000)
#define MByte(_v)  ((_v) * 1000 * 1000)
#define GByte(_v)  ((_v) * 1000 * 1000 * 1000)

#define KiByte(_v) ((_v) * 1024)
#define MiByte(_v) ((_v) * 1024 * 1024)
#define GiByte(_v) ((_v) * 1024 * 1024)

#define HUGE_PAGE  (MiByte(2))

namespace core::memory::memory {
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

    /**
     * @brief  Calculates the offset of a pointer in bytes needed to be aligned.
     * @tparam T   The type used to align to based from the given address.
     * @param  ptr The address from which we try to align a pointer.
     * @return The alignment in bytes as uintptr_t.
     */
    template <typename T>
    ALWAYS_INLINE
    auto ptr_offset(void *ptr) noexcept -> uintptr_t {
        constexpr const uintptr_t align = sizeof(T) - 1;
        return ((~reinterpret_cast<uintptr_t>(ptr)) + 1) & align;
    }
}

#endif //OPENGL_3D_ENGINE_MEMORY_H
