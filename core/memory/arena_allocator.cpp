//
// Created by Luis Ruisinger on 23.08.24.
//

#include "arena_allocator.h"

namespace core::memory::arena_allocator {
    static auto construct_page(size_t size) -> Result<memory::SLL<u8> *, memory::Error> {
        if (size < HUGE_PAGE) {
            return Err(memory::INVALID_PAGE_SIZE);
        }

        auto *ptr = new memory::SLL<Byte> {
                .size = size,
                .ptr = nullptr,
                .used = true,
                .next = nullptr
        };

        if (!ptr) {
            return Err(memory::ALLOC_FAILED);
        }

        // memory aligned to huge pages
        // because madvise operators on page aligned memory addresses
        posix_memalign(
                reinterpret_cast<void **>(&ptr->ptr),
                HUGE_PAGE,
                size);

        if (!ptr->ptr) {
            ::operator delete(ptr);
            return Err(memory::ALLOC_PAGE_FAILED);
        }

#ifdef __linux__
            madvise(reinterpret_cast<void **>(&ptr->ptr), size, MADV_HUGEPAGE);
#endif

        DEBUG_LOG(
                std::to_string(size / HUGE_PAGE),
                "pages allocated through posix_memalign");
        return Ok(ptr);
    }

    ArenaAllocator::ArenaAllocator()
            : list { nullptr }
    {
        ASSERT_NEQ(this->list.load(std::memory_order_relaxed));
        LOG("Arena allocator initialized")
    }

    ArenaAllocator::~ArenaAllocator() {
        destroy();
    }

    auto ArenaAllocator::destroy() -> void {
        memory::SLL<Byte> *page = list;

        while (page) {
            memory::SLL<Byte> *dealloc = page;
            page = page->next.load(std::memory_order_relaxed);

            ::operator delete(dealloc);
        }

        this->list.store(nullptr, std::memory_order_release);
    }

    auto ArenaAllocator::deallocate(const Byte *ptr, [[maybe_unused]] const size_t len) -> void {
        memory::SLL<Byte> *page = list.load(std::memory_order_relaxed);

        // linear search for page
        while (page && page->ptr != ptr) {
            page = page->next.load(std::memory_order_acquire);
        }

        // the page must exist
        ASSERT_EQ(page);
        ASSERT_EQ(page->ptr == ptr);
        page->used.store(false, std::memory_order_release);
    }

    auto ArenaAllocator::reuse_pages(memory::SLL<Byte> *page, size_t size) -> u8 * {
        // check if any allocated page from the last frame is unused
        // thus can be given a new owner
        while (page->next.load(std::memory_order_relaxed)) {

            auto used = page->used.load(std::memory_order_relaxed);
            if (used || page->size != size) {
                page = page->next;
                continue;
            }

            // a valid unused page has been found and acquired
            if (page->used.compare_exchange_weak(
                    used,
                    true,
                    std::memory_order_release,
                    std::memory_order_acquire)) {

                DEBUG_LOG("Reusable page found", page->ptr);
                ASSERT_EQ(page->ptr);
                return page->ptr;
            }

            page = page->next;
        }

        return nullptr;
    }

    auto ArenaAllocator::allocate(size_t size) -> Result<u8 *, memory::Error> {
        memory::SLL<Byte> *page = this->list.load(std::memory_order_acquire);

        // traversing all reset pages does only make sense if pages exist
        // this is not the case if the root is nullptr
        if (page) {
            auto candidate = reuse_pages(page, size);
            if (candidate)
                return Ok(candidate);
        }

        // in case no old pages are unused
        // allocate a new page and try to insert it into the linked list of pages
        auto ret = construct_page(size);
        if (ret.isErr()) {
            return Err(ret.unwrapErr());
        }

        auto *head = ret.unwrap();
        for (;;) {

            // fetching the root to see if it is initialized
            // if it is not the case we try to replace the root if a
            // new linked list entry
            page = list.load(std::memory_order_relaxed);

            if (page) [[likely]] {

                // in case the root is not nullptr we try to append
                // to the list head a new head
                for (;;) {
                    while (page->next.load(std::memory_order_acquire)) {
                        page = page->next.load(std::memory_order_acquire);
                    }

                    auto *next = page->next.load(std::memory_order_relaxed);
                    if (!page->next.compare_exchange_weak(
                            next,
                            head,
                            std::memory_order_release,
                            std::memory_order_acquire)) {
                        DEBUG_LOG("CAS failure");
                        continue;
                    }

                    return Ok(head->ptr);
                }
            }
            else {

                // try to replace the root if it is still nullptr
                if (!list.compare_exchange_strong(
                        page,
                        head,
                        std::memory_order_release,
                        std::memory_order_acquire)) {
                    DEBUG_LOG("Root has been set");
                    continue;
                }

                return Ok(head->ptr);
            }
        }
    }
}
