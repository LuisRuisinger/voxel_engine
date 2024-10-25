//
// Created by Luis Ruisinger on 21.05.24.
//

#ifndef OPENGL_3D_ENGINE_THREAD_SAFE_QUEUE_H
#define OPENGL_3D_ENGINE_THREAD_SAFE_QUEUE_H

#include <algorithm>
#include <concepts>
#include <deque>
#include <mutex>
#include <optional>
#include <future>
#include <atomic>

#include "../util/log.h"

namespace core::threading {
    template<typename Lock>
    concept is_lockable = requires(Lock&& lock) {
        lock.lock();
        lock.unlock();
        { lock.try_lock() } -> std::convertible_to<bool>;
    };

    template<typename T, typename Lock = std::mutex>
    requires is_lockable<Lock>
    class ThreadsafeQueue {
    public:

        /**
         * @brief  Tries to push an rvalue reference of T to the queue.
         *         If locking the mutex fails the rvalue wont get stored.
         * @param  t Rvalue reference of template parameter T.
         * @return Boolean indicating if pushing was successful.
         */
        auto try_push(T &&t) -> bool {
            {
                std::unique_lock<std::mutex> lock { this->mutex, std::try_to_lock };
                if (!lock)
                    return false;

                this->queue.emplace_back(std::forward<T>(t));
            }
            return true;
        }

        /**
         * @brief  Tries to pop an element from the queue. Assigns it via moving to the reference.
         * @param  t Reference to instance of template parameter T.
         * @return Boolean indicating if popping was successful.
         */
        auto try_pop(T &t) -> bool {
            {
                std::unique_lock<std::mutex> lock { this->mutex, std::try_to_lock };
                if (!lock || this->queue.empty())
                    return false;

                t = std::move(this->queue.front());
                this->queue.pop_front();
            }
            return true;
        }

        auto try_priority_push(T &&t) -> bool {
            {
                std::unique_lock<std::mutex> lock { this->mutex, std::try_lock };
                if (!lock)
                    return false;

                this->queue.push_front(std::forward<T>(t));
            }

            return true;
        }

        auto try_top() -> T * {
            return nullptr;
        }

    private:
        std::deque<T> queue;
        Lock mutex;
    };
}

#endif //OPENGL_3D_ENGINE_THREAD_SAFE_QUEUE_H