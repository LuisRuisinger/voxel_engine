//
// Created by Luis Ruisinger on 26.08.24.
//

#ifndef OPENGL_3D_ENGINE_SPMC_QUEUE_H
#define OPENGL_3D_ENGINE_SPMC_QUEUE_H

#include <cstdint>
#include <functional>
#include <atomic>

#include "../../util/defines.h"
#include "../../util/result.h"
#include "../../util/assert.h"

namespace core::threading::spmc_queue {

    template <typename T, unsigned Capacity = 512>
    class SPMCQueue {
    public:
        SPMCQueue()
            : buffer { std::unique_ptr<T[]> { new T[Capacity] } }
        {
            this->first.store(0, std::memory_order_relaxed);
            this->last.store(0, std::memory_order_relaxed);
        }

        ~SPMCQueue() =default;

        template <typename OT, unsigned OCapacity>
        SPMCQueue(const SPMCQueue<OT, OCapacity> &) =delete;

        template <typename OT, unsigned OCapacity>
        SPMCQueue(SPMCQueue<OT, OCapacity> &&) =delete;

        template <typename OT, unsigned OCapacity>
        auto operator=(const SPMCQueue<OT, OCapacity> &) -> SPMCQueue<OT, OCapacity> & =delete;

        template <typename OT, unsigned OCapacity>
        auto operator=(SPMCQueue<OT, OCapacity> &&) -> SPMCQueue<OT, OCapacity> & =delete;

        auto try_push(T &&t) -> bool {
            u32 _last = this->last.load(std::memory_order_relaxed);
            u32 _next = inc(last);

            if (_next == first.load(std::memory_order_acquire)) {
                return false;
            }

            last.store(_next, std::memory_order_release);
            this->buffer.get()[_last] = std::forward<T>(t);
            return true;
        }

        auto try_pop(T &t) -> bool {
            u32 _first = first.load(std::memory_order_relaxed);
            if (_first == last.load(std::memory_order_acquire)) {
                return false;
            }

            if (first.compare_exchange_weak(
                    _first,
                    inc(_first),
                    std::memory_order_release,
                    std::memory_order_acquire)) {
                t = std::move(this->buffer.get()[_first]);
                return true;
            }
            
            return false;
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
