#ifndef OPENGL_3D_ENGINE_LINEAR_ALLOCATOR_H
#define OPENGL_3D_ENGINE_LINEAR_ALLOCATOR_H

#include "memory.h"
#include "../util/result.h"

/**
 * @brief Because memory reclamation is deferred we need some kind of counter
 *        indicating unused memory over a long time span. If this counter would reach 0
 *        the underlying buffer is given back to the memory backend to be
 *        destroyed / cached / etc.
 */
#define DECAY 1024

/**
 * @brief The standard continous memory segment size the linear allocator manages and is
 *        able to split up in multiple smaller segments for actors to use.
 */
#define BUFFER_SIZE (HUGE_PAGE * 8)

/**
 * @brief Acquires the guard of the caller allocator. Used to ensure that during move
 *        operations or instance destruction no actor will be able to perform critical
 *        memory allocations.
 *        However it is not defined what would happen if an actor still tries to allocate
 *        or destroy memory after the instance has been destroyed.
 *        Allocations and destruction of memory after move operations are well defined
 *        and are happening in the scope of the new container.
 */
#define ACQUIRE_GUARD(_a) ({                                       \
    for (;;) {                                                     \
        bool is_allocating = (_a).load(std::memory_order_relaxed); \
        if (is_allocating)                                         \
            continue;                                              \
                                                                   \
        if ((_a).compare_exchange_weak(                            \
                is_allocating,                                     \
                true,                                              \
                std::memory_order_release,                         \
                std::memory_order_acquire))                        \
            break;                                                 \
        }})

namespace core::memory::linear_allocator {
    using Byte = u8;

    /**
     * @brief Metadata stored at the beginning of each page.
     *        Next points to the next page (if any) else null.
     *        Used is a counter indicating if the page has been in use.
     *        Head pointer to the next allocatable segment in this page.
     */
    struct Metadata {
        Byte *next;
        u32 used;
        std::atomic<Byte *> head;
    };

    /**
     * The linear allocator contains multiple equally sized pages on which other
     * containers can allocate memory. In case an allocation is not satisfied
     * (due to remaining size of a page) another page can be dynamically allocated
     * and appended to the other pages.
     *
     * pages
     * +-------------+    +-------------+    +-------------+    +-------------+
     * | single page | -> | single page | -> | single page | -> | single page | -> ..............
     * +-------------+    +-------------+    +-------------+    +-------------+
     * ^
     * |
     * page address stored by the allocator
     *
     * Allocations on a single page (possibly from multiple actors) do not know
     * anything about each other and can follow different types of alignment
     * (through different types being allocated).
     *
     * single page
     * +----------------------------+----------+------------------------------------------------+
     * | alignment to page metadata | metadata | ------------------- buffer ------------------- |
     * +----------------------------+----------+------------------------------------------------+
     * ^                                       ^
     * |                                       |
     * page address                            initial access head
     *
     * The metadata of a page contains a pointer to the next page (possibly null),
     * a counter indicating if the page this metadata belongs to has been in use recently
     * and an atomic access head which gets shifted on valid allocation.
     * If multiple actors are concurrently trying to allocate on a page only one is
     * guaranteed to succeed. All others would fetch the new moved access head and try
     * again to allocate on the page (if the now smaller remaining size of the buffer
     * satisfies the allocation). Else they traverse to the next first fitting page.
     * If no page satisfies the allocation a new page will be allocated using the allocator
     * hold by this linear allocator and appends another equally sized page.
     *
     * page segmentation
     * +---------------------------------+-------------------------------------+----------------+
     * | used page by through containers | ---------- new allocation --------- | remaining page |
     * +---------------------------------+-------------------------------------+----------------+
     * ^                                 ^                                     ^
     * |                                 |                                     |
     * access head before allocation     ptr to the allocation                 new access head
     *
     * structure of a new allocation
     * +----------------------+--------------------------------------------+--------------------+
     * | alignment to segment | ------------- usable segment ------------- |  remaining buffer  |
     * +----------------------+--------------------------------------------+--------------------+
     *                        ^                                            ^
     *                        |                                            |
     *                        ptr to the actual allocation                 new access head
     *
     * The segment will be shifted accordingly to alignment needed for the metadata,
     * the size of the metadata and the padding needed to align the writeable allocated area.
     * This means that an allocation does actually need a more space than length * alignment
     * of the allocation.
     *
     * @tparam Allocator The allocator being used to allocate pages this buffer linear segments.
     * @tparam Size      Size of all subsequent pages being allocated by this linear allocator.
     */
    template <
            typename Allocator = std::allocator<u8>,
            unsigned Size = BUFFER_SIZE>
    class LinearAllocator {

        // checks if the allocation API returns a Result type
        template <typename T, typename ...Args>
        using allocator_returns_result = std::is_same<
                decltype(std::declval<T>().allocate(std::declval<Args>()...)),
                Result<typename Allocator::value_type *, memory::Error>>;

        // checks if the allocation API returns a u8 pointer
        template <typename T, typename ...Args>
        using allocator_returns_ptr = std::is_same<
                decltype(std::declval<T>().allocate(std::declval<Args>()...)),
                typename Allocator::value_type *>;

        // unknown allocator type is in use
        // the standard allocator and arena allocator both satisfy
        // the condition to not throw
        static_assert(
                allocator_returns_result<Allocator, u64>::value ||
                allocator_returns_ptr<Allocator, size_t>::value);

    public:

        /**
         * @brief Initializes an bump allocator of size capacity bytes.
         * @param capacity Underlying buffer size in bytes.
         * @post  Allocated managed buffer of size capacity bytes.
         */
        explicit LinearAllocator(Allocator *allocator)
                : allocator { allocator },
                  appending { false     }
        {
            Byte *ptr = nullptr;

            if constexpr (allocator_returns_result<Allocator, u64>::value) {
                const auto res = this->allocator->allocate(Size);

                if (res.isErr()) {
                    LOG(util::log::Level::LOG_LEVEL_ERROR, res.unwrapErr());
                    std::exit(EXIT_FAILURE);
                }
                else {
                    ptr = res.unwrap();
                }
            }

            if constexpr (allocator_returns_ptr<Allocator, u64>::value) {
                ptr = this->allocator->allocate(Size);
                if (!ptr) {
                    std::exit(EXIT_FAILURE);
                }
            }

            // unknown allocator type is in use
            static_assert(
                    allocator_returns_result<Allocator, u64>::value ||
                    allocator_returns_ptr<Allocator, u64>::value);

            this->memory = ptr;

            // init metadata for the first page
            const uintptr_t metadata_pad = memory::ptr_offset<Metadata>(ptr);
            auto *metadata = reinterpret_cast<Metadata *>(ptr + metadata_pad);

            metadata->next = nullptr;
            metadata->used = DECAY;
            metadata->head.store(
                    static_cast<u8 *>(ptr + metadata_pad + sizeof(Metadata)),
                    std::memory_order_relaxed);

            LOG("Page size", std::to_string(Size));
            LOG("Page allocated at", this->memory);
            LOG("Head at", metadata->head.load(std::memory_order_relaxed));
            LOG("Last valid address at", this->memory + (Size - 1));
        }

        template <typename OAllocator, unsigned OSize>
        explicit LinearAllocator(LinearAllocator<OAllocator, OSize> &&other) noexcept {
            ACQUIRE_GUARD(this->appending);
            ACQUIRE_GUARD(other.appending);

            this->allocator = other.allocator;
            this->memory = other.memory;
            other.memory = nullptr;

            this->appending.store(false, std::memory_order_release);
            other.appending.store(false, std::memory_order_release);
        }

        template <typename OAllocator, unsigned OSize>
        auto operator=(LinearAllocator<OAllocator, OSize> &&other) noexcept
                -> LinearAllocator<OAllocator, OSize> & {
            ACQUIRE_GUARD(this->appending);
            ACQUIRE_GUARD(other.appending);

            this->allocator = other.allocator;
            this->memory = other.memory;
            other.memory = nullptr;

            this->appending.store(false, std::memory_order_release);
            other.appending.store(false, std::memory_order_release);
            return *this;
        }

        template <typename OAllocator, unsigned OSize>
        explicit LinearAllocator(const LinearAllocator<OAllocator, OSize> &) =delete;

        template <typename OAllocator, unsigned OSize>
        auto operator=(const LinearAllocator<OAllocator, OSize> &)
        -> LinearAllocator<OAllocator, OSize> & =delete;

        ~LinearAllocator() {
            u8 *page = this->memory;

            while (page) {
                const uintptr_t pad =  memory::ptr_offset<Metadata>(page);
                const auto *metadata = reinterpret_cast<Metadata *>(page + pad);

                Byte *dealloc = page;
                page = metadata->next;

                this->allocator->deallocate(dealloc, Size);
            }
        }

        /**
         * @brief  Tries to allocate a segment of len * align.
         *         If the buffers capacity doesn't suffice a new buffer will be allocated.
         *         The segment will be shifted over to the new buffer.
         * @param  len Amount of elements of the specified alignment the allocation should hold.
         * @param  align The size of the type which the allocation should hold.
         * @return Address to a buffer containing the size of allocated segment,
         *         the actual used size (writing to the segment) and the segment
         *         stating where the accessible segment starts in memory.
         */
        template <typename T>
        auto allocate(size_t len) -> T * {
            Byte *page = this->memory;

            for (;;) {
                const uintptr_t pad = memory::ptr_offset<Metadata>(page);
                auto *metadata = reinterpret_cast<Metadata *>(page + pad);

                for (;;) {
                    Byte *head = metadata->head.load(std::memory_order_relaxed);

                    const uintptr_t buffer_padding = memory::ptr_offset<T>(head);
                    const uintptr_t max_size = buffer_padding + len * sizeof(T);

                    // check if the remaining size of the page satisfies
                    // the actual size of the allocation (all padding, metadata and len * alignment)
                    if (reinterpret_cast<uintptr_t>(head) + max_size >
                        reinterpret_cast<uintptr_t>(page) + Size) {
                        break;
                    }

                    if (!metadata->head.compare_exchange_weak(
                            head,
                            head + max_size,
                            std::memory_order_release,
                            std::memory_order_acquire)) {
                        DEBUG_LOG("CAS failed");
                        continue;
                    }
#ifdef DEBUG
                    const uintptr_t diff =
                            reinterpret_cast<uintptr_t>(head + buffer_padding) +
                            (len * sizeof(T)) -
                            reinterpret_cast<uintptr_t>(page);
                    ASSERT_EQ(diff <= Size);
#endif
                    return reinterpret_cast<T *>(head + buffer_padding);
                }

                // we need to continue instead of directly jumping to the new buffer
                // because req_memory doesn't guarantee that another thread is already
                // allocating memory which would instantly return all other threads from
                // this function
                if (!metadata->next) {
                    req_memory(page);
                    continue;
                }

                page = metadata->next;
            }
        }

        /** @brief Resets all used pages.
         *         Pages that haven't been used for a long time are reclaimed
         *         by the allocator. The allocator decides what to do with these
         *         pages (e.g. buffering).
         *  @post  Each head at each page is set to the first allocatable address
         *         on its page.
         *
         *  This guarantees that at least one page still remains usable by
         *  this allocator.
         */
        auto reset() -> void {
            Byte *last_valid_page = nullptr;
            Byte *page = this->memory;

            // traverse as long as we hit the first unused page
            while (page) {
                const uintptr_t pad = memory::ptr_offset<Metadata>(page);
                auto *metadata = reinterpret_cast<Metadata *>(page + pad);

                // page is unused
                // guarantees that at least one page remains
                if (metadata->used == 0 && last_valid_page)
                    break;

                // check if the head has been moved
                Byte *head = reinterpret_cast<Byte *>(metadata) + sizeof(Metadata);
                metadata->used =
                        (metadata->head != head)
                            ? DECAY
                            : metadata->used - 1;

                // reset linear head
                metadata->head = head;

                // swap
                last_valid_page = page;
                page = metadata->next;
            }

            // deallocate every page after first hit
            ASSERT_EQ(page != this->memory);
            while (page) {
                const uintptr_t pad = memory::ptr_offset<Metadata>(page);
                const auto *metadata = reinterpret_cast<Metadata *>(page + pad);

                Byte *dealloc = page;
                page = metadata->next;

                this->allocator->deallocate(dealloc, Size);
            }

            // the last valid page loses reference to its following pages
            const uintptr_t pad = memory::ptr_offset<Metadata>(last_valid_page);
            reinterpret_cast<Metadata *>(last_valid_page + pad)->next = nullptr;
        }

        /**
         * @brief Allocates a new page from the memory backend.
         *        Ensures only one actor can actually request and append a new page.
         *        The page size is static and the same for all pages.
         * @param page The last valid page this allocator can use and offer actors.
         *             The new page will be appended into the metadata of this page.
         * @post  The allocator's linked list of pages will contain a new, unused and valid
         *        page at the end of the list. Actors are able to allocate regions on this new
         *        page.
         */
        auto req_memory(Byte *page) -> void {
            bool is_allocating = this->appending.load(std::memory_order_acquire);
            if (is_allocating)
                return;

            // some thread try to gain the flag to allocate at the same time
            // might fail due to spurious failure
            if (!this->appending.compare_exchange_weak(
                    is_allocating,
                    true,
                    std::memory_order_release,
                    std::memory_order_acquire)) {
                return;
            }

            Byte *ptr = nullptr;
            if constexpr (allocator_returns_result<Allocator, u64>::value) {
                const auto res = this->allocator->allocate(Size);

                if (res.isErr()) {
                    LOG(util::log::Level::LOG_LEVEL_ERROR, res.unwrapErr());
                    std::exit(EXIT_FAILURE);
                }
                else {
                    ptr = res.unwrap();
                }
            }

            if constexpr (allocator_returns_ptr<Allocator, u64>::value) {
                ptr = this->allocator->allocate(Size);

                if (!ptr) {
                    std::exit(EXIT_FAILURE);
                }
            }

            // appending the new page
            const uintptr_t pad = memory::ptr_offset<Metadata>(page);
            reinterpret_cast<Metadata *>(page + pad)->next = ptr;

            // init metadata for the new page
            const uintptr_t metadata_pad = memory::ptr_offset<Metadata>(ptr);
            auto *metadata_ptr = reinterpret_cast<Metadata *>(ptr + metadata_pad);

            metadata_ptr->next = nullptr;
            metadata_ptr->used = DECAY;
            metadata_ptr->head.store(
                    static_cast<Byte *>(ptr + metadata_pad + sizeof(Metadata)),
                    std::memory_order_relaxed);

            // releasing the flag
            this->appending.store(false, std::memory_order_release);

            LOG("Page allocated at", ptr);
            LOG("Head at", metadata_ptr->head.load(std::memory_order_relaxed));
            LOG("Last valid address at", ptr + (Size - 1));
        }

    private:

        /** @brief Allocator to request pages. */
        Allocator *allocator;

        /**
         * @brief Raw pointer addressing the start of the first page.
         *        Metadata and per allocation data are embedded in each page.
         */
        Byte *memory;

        /** @brief Indicating if a thread is currently requesting a new page */
        alignas(CACHE_LINE_SIZE) std::atomic<bool> appending;
    };
}


#endif //OPENGL_3D_ENGINE_LINEAR_ALLOCATOR_H
