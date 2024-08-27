//
// Created by Luis Ruisinger on 26.08.24.
//

#ifndef OPENGL_3D_ENGINE_SPMC_QUEUE_H
#define OPENGL_3D_ENGINE_SPMC_QUEUE_H

#include <cstdint>
#include <functional>
#include <atomic>

#include "../../util/aliases.h"
#include "../../util/result.h"
#include "../../util/assert.h"

namespace core::threading::spmc_queue {

    template <typename T, unsigned Capacity = 128>
    class SPMCQueue {
    public:
        SPMCQueue()
                : buffer { std::unique_ptr<T[]> { new T[Capacity] } } {
            this->first.store(0, std::memory_order_relaxed);
            this->last.store(0, std::memory_order_relaxed);
        }

        ~SPMCQueue() =default;

        template <typename _T, unsigned _Capacity>
        SPMCQueue(const SPMCQueue<_T, _Capacity> &) =delete;

        template <typename _T, unsigned _Capacity>
        SPMCQueue(SPMCQueue<_T, _Capacity> &&) =delete;

        template <typename _T, unsigned _Capacity>
        auto operator=(const SPMCQueue<_T, _Capacity> &) -> SPMCQueue<_T, _Capacity> & =delete;

        template <typename _T, unsigned _Capacity>
        auto operator=(SPMCQueue<_T, _Capacity> &&) -> SPMCQueue<_T, _Capacity> & =delete;

        auto try_push(T &&t) -> bool {
            u32 lst = this->last.load(std::memory_order_relaxed);
            u32 nxt = inc(last);

            if (nxt == first.load(std::memory_order_acquire))
                return false;

            last.store(nxt, std::memory_order_release);
            this->buffer.get()[lst] = std::move(t);
            return true;
        }

        auto try_pop(T &t) -> bool {
            for (;;) {
                u32 fst = first.load(std::memory_order_relaxed);
                if (fst == last.load(std::memory_order_acquire))
                    return false;

                if (first.compare_exchange_weak(
                        fst,
                        inc(fst),
                        std::memory_order_release,
                        std::memory_order_acquire)) {
                    t = std::move(this->buffer.get()[fst]);
                    return true;
                }
            }
        }

    private:
        auto inline inc(u32 n) -> u32 {
            return (n + 1) % Capacity;
        }

        // non-aligned to CACHE_LINE_SIZE because you always need to
        // access both of them for a decision
        std::atomic<u32> first;
        std::atomic<u32> last;

        // access should not contain shared atomics
        alignas(CACHE_LINE_SIZE) std::unique_ptr<T[]> buffer;
    };
}


#endif //OPENGL_3D_ENGINE_SPMC_QUEUE_H
