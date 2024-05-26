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

namespace core::threading {
    template<typename Lock>
    concept is_lockable = requires(Lock&& lock) {
        lock.lock();
        lock.unlock();
        { lock.try_lock() } -> std::convertible_to<bool>;
    };

    //
    //
    //
    //
    //

    template<typename T, typename Lock = std::mutex>
    requires is_lockable<Lock>
    class ThreadsafeQueue {
    public:

        /**
         * @brief Tries to push an rvalue reference of T to the queue.
         *        If locking the mutex fails the rvalue wont get stored.
         *
         * @param t Rvalue reference of template parameter T.
         *
         * @return Boolean indicating if pushing was successful.
         */

        auto try_push(T &&t) -> bool {
            {
                std::unique_lock<std::mutex> lock{_mutex, std::try_to_lock};
                if (!lock)
                    return false;

                _queue.emplace_back(std::forward<T>(t));
            }
            return true;
        }

        /**
         * @brief Tries to pop an element from the queue. Assigns it via moving to the reference.
         *
         * @param t Reference to instance of template parameter T.
         *
         * @return Boolean indicating if popping was successful.
         */

        auto try_pop(T &t) -> bool {
            {
                std::unique_lock<std::mutex> lock{_mutex, std::try_to_lock};
                if (!lock || _queue.empty())
                    return false;

                t = std::move(_queue.front());
                _queue.pop_front();
            }
            return true;
        }

    private:
        std::deque<T> _queue;
        Lock          _mutex;
    };
}

#endif //OPENGL_3D_ENGINE_THREAD_SAFE_QUEUE_H