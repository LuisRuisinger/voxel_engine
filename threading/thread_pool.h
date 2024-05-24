//
// Created by Luis Ruisinger on 21.05.24.
//

#ifndef OPENGL_3D_ENGINE_THREAD_POOL_H
#define OPENGL_3D_ENGINE_THREAD_POOL_H

#include <atomic>
#include <barrier>
#include <concepts>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <semaphore>
#include <thread>
#include <type_traits>
#include <iostream>

#include "thread_safe_queue.h"
#include "../util/aliases.h"

#define WRAPPED_EXEC(_f) { try { (_f); } catch(...) {} }

namespace core::threading {
    namespace functor {
#ifdef __cpp_lib_move_only_function
        using default_function_type = std::move_only_function<void()>;
#else
        using default_function_type = std::function<void()>;
#endif
    }

    template<typename Function_type = functor::default_function_type>
    requires std::invocable<Function_type> &&
             std::is_same_v<void, std::invoke_result_t<Function_type>>
    class Tasksystem {
    public:
        explicit Tasksystem(u32 thread_count = std::thread::hardware_concurrency())
            : _count{thread_count}
        {
            for (size_t i = 0; i < _count; ++i)
                _threads.emplace_back([&, i] { run(i); });
            _running = true;
        }

        ~Tasksystem() {
            _running = false;
            _ready.notify_all();

            for (auto &t : _threads)
                if (t.joinable()) t.join();
        }

        Tasksystem(const Tasksystem &) =delete;
        Tasksystem(Tasksystem &&) noexcept =default;

        auto operator=(const Tasksystem &) -> Tasksystem & =delete;
        auto operator=(Tasksystem &&) noexcept -> Tasksystem & =default;

        template <typename F>
        auto try_schedule(F &&f) -> bool {
            u32 i = _index++;

            for (size_t n = 0; n < _count; ++n) {
                if (_queues[(i + n) % _count].try_push(std::forward<F>(f))) {
                    _enqueued    += 1;
                    _active_taks += 1;

                    _ready.notify_one();
                    return true;
                }
            }

            if (_queues[i % _count].try_push(std::forward<F>(f))) {
                _enqueued    += 1;
                _active_taks += 1;

                _ready.notify_one();
                return true;
            }

            return false;
        }

        /**
         * @brief Enqueue a task that returns void
         *
         * @tparam F An invokable type
         * @tparam Args Argument pack for F
         *
         * @param fun The to be executed callable
         * @param args Arguments that will be passed to fun
         */

        template<typename F, typename ...Args>
        requires std::invocable<F, Args...> &&
                 std::is_same_v<void, std::invoke_result_t<F &&, Args &&...>>
        auto enqueue_detach(F &&fun, Args &&...args) -> void {
            while (!try_schedule(std::move([fun = std::forward<F>(fun),
                                            ...args = std::forward<Args>(args)]() mutable -> decltype(auto) {
                WRAPPED_EXEC(std::invoke(fun, args...));
            }))) {}
        }

        /**
         * @brief Calling thread waits until every queued task has been finished
         *
         * @param timeout Timeout after which the calling thread returns. Defaulted to 5ms.
         * If set to 0 no timeout will be applied.
         */

        auto wait_for_tasks(std::chrono::milliseconds timeout = std::chrono::milliseconds(5)) -> void {
            std::unique_lock<std::mutex> lock{_mutex};
            if (timeout.count()) {
                _batch_finished.wait_for(lock, timeout, [this] { return _enqueued == 0 && _active_taks == 0; });
            }
            else {
                _batch_finished.wait(lock, [this] { return _enqueued == 0 && _active_taks == 0; });
            }
        }

        auto no_tasks() -> bool {
            return _enqueued == 0 & _active_taks == 0;
        }

    private:
        auto run(u32 i) -> void {
            while (_running || _enqueued > 0) {
                bool dequeued = false;
                Function_type task;

                {
                    std::unique_lock<std::mutex> lock{_mutex};
                    _ready.wait(lock, [this] { return _enqueued > 0 || !_running; });
                }

                while (!dequeued) {
                    for (size_t n = 0; n != _count; ++n) {
                        if (_queues[(i + n) % _count].try_pop(task)) {
                            dequeued = true;
                            if (_enqueued > 0)
                                _enqueued -= 1;

                            task();

                            _active_taks -= 1;
                            if (_enqueued == 0 && _active_taks == 0)
                                _batch_finished.notify_all();

                            break;
                        }
                    }

                    if (!task && _queues[i].try_pop(task)) {
                        dequeued = true;
                        if (_enqueued > 0)
                            _enqueued -= 1;

                        task();

                        _active_taks -= 1;
                        if (_enqueued == 0 && _active_taks == 0)
                            _batch_finished.notify_all();
                    }

                    if (!(_running || _enqueued > 0))
                        break;
                }
            }
        }

        const u32                                   _count;
        std::vector<std::thread>                    _threads;
        std::vector<ThreadsafeQueue<Function_type>> _queues{_count};

        std::atomic_size_t _enqueued{0};
        std::atomic_size_t _active_taks{0};
        std::atomic_size_t _index{0};
        std::atomic_bool   _running{false};

        std::mutex _mutex;

        std::condition_variable _ready;
        std::condition_variable _batch_finished;
    };


}

#endif //OPENGL_3D_ENGINE_THREAD_POOL_H
