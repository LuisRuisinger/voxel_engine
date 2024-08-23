//
// Created by Luis Ruisinger on 01.08.24.
//

#ifndef OPENGL_3D_ENGINE_LINEAR_ALLOCATOR_THREADSAFE_H
#define OPENGL_3D_ENGINE_LINEAR_ALLOCATOR_THREADSAFE_H

#include "memory.h"
#include "../../util/result.h"

#define DECAY       (60)
#define BUFFER_SIZE (HUGE_PAGE * 8)
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

    /**
     * @brief Metadata stored at the beginning of each page.
     *        Next points to the next page (if any) else null.
     *        Used is a counter indicating if the page has been in use.
     *        Head pointer to the next allocatable segment in this page.
     */
    struct Metadata {
        u8 *next;
        u8 used;
        std::atomic<u8 *> head;
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
    class BumpAllocator {

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
        explicit BumpAllocator(Allocator *allocator)
                : allocator { allocator },
                  appending { false     }
        {
            u8 *ptr = nullptr;

            if constexpr (allocator_returns_result<Allocator, u64>::value) {
                const auto res = this->allocator->allocate(Size);

                if (res.isErr())
                    util::log::out() << util::log::Level::LOG_LEVEL_ERROR
                                     << res.unwrapErr()
                                     << util::log::end;
                ptr = res.unwrap();
            }

            if constexpr (allocator_returns_ptr<Allocator, u64>::value) {
                ptr = this->allocator->allocate(Size);
                if (!ptr) std::exit(EXIT_FAILURE);
            }

            // unknown allocator type is in use
            static_assert(
                    allocator_returns_result<Allocator, u64>::value ||
                    allocator_returns_ptr<Allocator, u64>::value);

            this->memory = ptr;

            // init metadata for the first page
            const u64 metadata_pad = memory::calculate_padding(ptr, sizeof(Metadata));
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
        explicit BumpAllocator(BumpAllocator<OAllocator, OSize> &&other) noexcept {
            ACQUIRE_GUARD(this->appending);
            ACQUIRE_GUARD(other.appending);

            this->allocator = other.allocator;
            this->memory = other.memory;
            other.memory = nullptr;

            this->appending.store(false, std::memory_order_release);
            other.appending.store(false, std::memory_order_release);
        }

        template <typename OAllocator, unsigned OSize>
        auto operator=(BumpAllocator<OAllocator, OSize> &&other) noexcept
                -> BumpAllocator<OAllocator, OSize> & {
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
        explicit BumpAllocator(const BumpAllocator<OAllocator, OSize> &) =delete;

        template <typename OAllocator, unsigned OSize>
        auto operator=(const BumpAllocator<OAllocator, OSize> &)
        -> BumpAllocator<OAllocator, OSize> & =delete;

        ~BumpAllocator() {
            u8 *page = this->memory;

            while (page) {
                const u64 pad = memory::calculate_padding(page, sizeof(Metadata));
                const auto *metadata = reinterpret_cast<Metadata *>(page + pad);

                u8 *dealloc = page;
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
        auto allocate(u64 len) -> T * {
            u8 *page = this->memory;

            for (;;) {
                const u64 pad = memory::calculate_padding(page, sizeof(Metadata));
                auto *metadata = reinterpret_cast<Metadata *>(page + pad);
                ASSERT_NEQ(
                        reinterpret_cast<u64>(metadata) % sizeof(Metadata),
                        "missaligned page metadata");

                for (;;) {
                    u8 *head = metadata->head.load(std::memory_order_relaxed);
                    const u64 buffer_padding =
                            memory::calculate_padding(head, sizeof(T));
                    const u64 max_size = buffer_padding + len * sizeof(T);

                    // check if the remaining size of the page satisfies
                    // the actual size of the allocation (all padding, metadata and len * alignment)
                    if (reinterpret_cast<u64>(head) + max_size >
                        reinterpret_cast<u64>(page) + Size)
                        break;

                    if (!metadata->head.compare_exchange_weak(head,
                                                              head + max_size,
                                                              std::memory_order_release,
                                                              std::memory_order_acquire)) {
                        DEBUG_LOG("CAS failed");
                        continue;
                    }
#ifdef DEBUG
                    const u64 diff =
                            reinterpret_cast<u64>(head + buffer_padding) + len * sizeof(T) -
                            reinterpret_cast<u64>(page);
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
            u8 *last_valid_page = nullptr;
            u8 *page = this->memory;

            // traverse as long as we hit the first unused page
            while (page) {
                const u64 pad = memory::calculate_padding(page, sizeof(Metadata));
                auto *metadata = reinterpret_cast<Metadata *>(page + pad);

                // page is unused
                // guarantees that at least one page remains
                if (metadata->used == 0 && last_valid_page)
                    break;

                // check if the head has been moved
                u8 *head = reinterpret_cast<u8 *>(metadata) + sizeof(Metadata);
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
                const u64 pad = memory::calculate_padding(page, sizeof(Metadata));
                const auto *metadata = reinterpret_cast<Metadata *>(page + pad);

                u8 *dealloc = page;
                page = metadata->next;

                this->allocator->deallocate(dealloc, Size);
            }

            // the last valid page loses reference to its following pages
            const u64 pad = memory::calculate_padding(last_valid_page, sizeof(Metadata));
            auto *metadata = reinterpret_cast<Metadata *>(last_valid_page + pad);
            metadata->next = nullptr;
        }

        auto req_memory(u8 *page) -> void {
            bool is_allocating = this->appending.load(std::memory_order_acquire);
            if (is_allocating)
                return;

            // some thread try to gain the flag to allocate at the same time
            // might fail due to spurious failure
            if (!this->appending.compare_exchange_weak(is_allocating,
                                                       true,
                                                       std::memory_order_release,
                                                       std::memory_order_acquire)) {
                return;
            }

            u8 *ptr = nullptr;
            if constexpr (allocator_returns_result<Allocator, u64>::value) {
                const auto res = this->allocator->allocate(Size);

                if (res.isErr())
                    util::log::out() << util::log::Level::LOG_LEVEL_ERROR
                                     << res.unwrapErr()
                                     << util::log::end;
                ptr = res.unwrap();
            }

            if constexpr (allocator_returns_ptr<Allocator, u64>::value) {
                ptr = this->allocator->allocate(Size);
                if (!ptr) std::exit(EXIT_FAILURE);
            }

            // appending the new page
            const u64 pad = memory::calculate_padding(page, sizeof(Metadata));
            auto *metadata = reinterpret_cast<Metadata *>(page + pad);
            metadata->next = ptr;

            // init metadata for the new page
            const u64 metadata_pad = memory::calculate_padding(ptr, sizeof(Metadata));
            auto *metadata_ptr = reinterpret_cast<Metadata *>(ptr + metadata_pad);

            metadata_ptr->next = nullptr;
            metadata_ptr->used = DECAY;
            metadata_ptr->head.store(
                    static_cast<u8 *>(ptr + metadata_pad + sizeof(Metadata)),
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
        u8 *memory;

        /** @brief Indicating if a thread is currently requesting a new page */
        std::atomic<bool> appending;
    };
}


#endif //OPENGL_3D_ENGINE_LINEAR_ALLOCATOR_THREADSAFE_H
