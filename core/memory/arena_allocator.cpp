//
// Created by Luis Ruisinger on 23.08.24.
//

#ifdef _WIN32
    #define WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <stdlib.h> 
    #include <sys/mman.h> 
#endif

#include "arena_allocator.h"

namespace core::memory::arena_allocator {
    auto ArenaAllocator::construct_page(usize size) -> SLL<u8> * {
        ASSERT_EQ(IS_POW_2(HUGE_PAGE));
        size = (size + HUGE_PAGE - 1) & -HUGE_PAGE;

        // new throws std::bad_alloc
        SLL<Byte> *ptr;
        try {
            ptr = new SLL<Byte> {
                .size = size,
                .ptr = nullptr,
                .used = true,
                .next = nullptr
            };
        }
        catch (std::exception &err) {
            LOG(memory::Error::ALLOC_FAILED);
            return nullptr;
        }

    #ifdef _WIN32
        ptr->ptr = reinterpret_cast<Byte*>(VirtualAlloc(
            nullptr,                       
            size,                    
            MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES, 
            PAGE_READWRITE));            

        if (!ptr->ptr) {
            ::operator delete(ptr);
            return Err(memory::ALLOC_PAGE_FAILED);
        }

    #else
        posix_memalign(reinterpret_cast<void **>(&ptr->ptr), HUGE_PAGE, size);

        if (!ptr->ptr) {
            delete ptr;
            return nullptr;
        }

    #ifdef __linux__
        madvise(reinterpret_cast<void **>(&ptr->ptr), size, MADV_HUGEPAGE);
    #endif

    #endif
        DEBUG_LOG(
            std::to_string(size / HUGE_PAGE), 
            "pages allocated through platform-specific method");
        return ptr;
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
        SLL<Byte> *page = list;

        while (page) {
            SLL<Byte> *dealloc = page;
            page = page->next.load(std::memory_order_relaxed);

            ::operator delete(dealloc);
        }

        this->list.store(nullptr, std::memory_order_release);
    }

    auto ArenaAllocator::deallocate(const Byte *ptr, [[maybe_unused]] const usize len) -> void {
        SLL<Byte> *page = list.load(std::memory_order_relaxed);

        // linear search for page
        while (page && page->ptr != ptr) {
            page = page->next.load(std::memory_order_acquire);
        }

        // the page must exist
        ASSERT_EQ(page);
        ASSERT_EQ(page->ptr == ptr);
        page->used.store(false, std::memory_order_release);
    }

    auto ArenaAllocator::reuse_pages(SLL<Byte> *page, usize size) -> Byte * {
        
        // check if any allocated page from the last frame is unused
        // thus can be given a new owner
        while (page) {
            ASSERT_EQ(page);
            
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

            page = page->next.load(std::memory_order_relaxed);
        }

        return nullptr;
    }

    auto ArenaAllocator::allocate(usize size) -> Byte * {
        SLL<Byte> *page = this->list.load(std::memory_order_acquire);
        if (page) {
            auto candidate = reuse_pages(page, size);
            if (candidate)
                return candidate;
        }

        // in case no old pages are unused
        // allocate a new page and try to insert it into the linked list of pages
        const auto head = construct_page(size);
        if (!head) {
            return nullptr;
        }

        // try to replace the root if it is still nullptr
        if (!page) {
            if (list.compare_exchange_strong(
                    page,
                    head,
                    std::memory_order_release,
                    std::memory_order_acquire)) {
                return head->ptr;
            }
        }

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
                continue;
            }

            return head->ptr;
        }
    }
}
