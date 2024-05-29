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

        /**
         * @brief Thread pool constructor.
         *
         * @param thread_count Amount of threads this instance should hold.
         *                     Defaults number of concurrent threads supported by the implementation.
         */

        explicit Tasksystem(u32 thread_count = std::thread::hardware_concurrency())
            : _count{thread_count}
            , _queues{thread_count}
            , _enqueued{0}
            , _active_tasks{0}
            , _index{0}
            , _running{false}
        {
            for (size_t i = 0; i < _count; ++i)
                _threads.emplace_back([&, i] { run(i); });
            _running = true;
        }

        /**
         * @brief Thread pool destructor, joining all threads.
         */

        ~Tasksystem() {
            _running = false;
            _ready.notify_all();

            for (auto &t : _threads)
                if (t.joinable()) t.join();
        }

        /**
         * @brief Try to schedule a task into the thread pool.
         *
         * @tparam F An invokable type.
         *
         * @param f The to be executed callable.
         */

        template <typename F>
        auto try_schedule(F &&f) -> bool {
            u32 i = _index++;

            // tries to schedule for the first unlocked thread
            // in case all fail we try again for the actual main thread we started with
            for (size_t n = 0; n < _count + 1; ++n) {
                if (_queues[(i + n) % _count].try_push(std::forward<F>(f))) {
                    _enqueued.fetch_add(1, std::memory_order_relaxed);
                    _active_tasks.fetch_add(1, std::memory_order_relaxed);

                    _ready.notify_one();
                    return true;
                }
            }

            return false;
        }

        /**
         * @brief Enqueue a task that returns void.
         *
         * @tparam F An invokable type.
         * @tparam Args Argument pack for F.
         *
         * @param fun The to be executed callable.
         * @param args Arguments that will be passed to fun.
         */

        template<typename F, typename ...Args>
        requires std::invocable<F, Args...> &&
                 std::is_same_v<void, std::invoke_result_t<F &&, Args &&...>>
        auto enqueue_detach(F &&fun, Args &&...args) -> void {
            auto task = [fun = std::forward<F>(fun), ...args = std::forward<Args>(args)]() mutable -> decltype(auto) {
                    WRAPPED_EXEC(std::invoke(fun, args...));
            };

            // trying until scheduled
            while (!try_schedule(std::move(task))) {}
        }

        /**
         * @brief Enqueue a task with async result.
         *
         * @tparam F An invokable type.
         * @tparam Args Argument pack for F.
         *
         * @param fun The to be executed callable.
         * @param args Arguments that will be passed to fun.
         *
         * @return A future of the result of fun.
         */

        template<typename F, typename ...Args , typename Ret = std::invoke_result_t<F &&, Args &&...>>
        requires std::invocable<F, Args...>
        auto enqueue(F &&fun, Args &&...args) -> std::future<Ret> {
#ifdef __cpp_lib_move_only_function
            auto promise = std::promise<Ret>(packaged_task.getfuture());
            auto future  = promise.get_future();

            auto task = [func = std::move(f), ... largs = std::move(args), promise = std::move(promise)]() mutable {
                try {
                    if constexpr (std::is_same_v<ReturnType, void>) {
                        func(largs...);
                        promise.set_value();
                    }
                    else {
                        promise.set_value(func(largs...));
                    }
                } catch (...) {
                    promise.set_exception(std::current_exception());
                }
            };
#else
            auto packaged_task =
                    std::packaged_task<Ret>(std::bind(std::forward<F>(fun),std::forward<Args>(args)...));

            auto promise = std::make_shared<std::promise<Ret>>(packaged_task.getfuture());
            auto future  = promise->get_future();

            auto task = [packaged_task, promise]() mutable -> void {
                try {
                    if constexpr (std::is_same_v<Ret, void>) {
                        packaged_task();
                        promise->set_value();
                    }
                    else {
                        promise->set_value(packaged_task());
                    }
                }
                catch (...) {
                    promise->set_exception(std::current_exception());
                }
            };

#endif
            // trying until scheduled
            while (!try_schedule(std::move(task))) {}
            return future;
        }

        /**
         * @brief Calling thread waits until every queued task has been finished.
         *
         * @param timeout Timeout after which the calling thread returns. Defaulted to 5ms.
         *                If set to 0 no timeout will be applied.
         */

        auto wait_for_tasks(std::chrono::milliseconds timeout = std::chrono::milliseconds(5)) -> void {
            std::unique_lock<std::mutex> lock{_mutex};
            if (timeout.count()) {
                _batch_finished.wait_for(lock, timeout, [this] {
                    return _enqueued.load(std::memory_order_acquire) == 0 &&
                           _active_tasks.load(std::memory_order_acquire) == 0; });
            }
            else {
                _batch_finished.wait(lock, [this] {
                    return _enqueued.load(std::memory_order_acquire) == 0 &&
                           _active_tasks.load(std::memory_order_acquire) == 0; });
            }
        }

        /**
         * @brief Check if all tasks have been finished and no unscheduled work remains. Non-blocking.
         *
         * @return Boolean value to indicate if batch is finished.
         */

        auto no_tasks() -> bool {
            return _enqueued.load(std::memory_order_acquire) == 0 &&
                   _active_tasks.load(std::memory_order_acquire) == 0;
        }

        Tasksystem(const Tasksystem &) =delete;
        Tasksystem(Tasksystem &&) noexcept =default;

        auto operator=(const Tasksystem &) -> Tasksystem & =delete;
        auto operator=(Tasksystem &&) noexcept -> Tasksystem & =default;

    private:

        /**
         * @brief Runnable wrapper function for each worker.
         *
         * @param i The index of the thread inside the task system.
         */

        auto run(u32 i) -> void {
            while (_running || _enqueued > 0) {
                bool dequeued = false;
                Function_type task;

                {
                    std::unique_lock<std::mutex> lock{_mutex};
                    _ready.wait(lock, [this] { return _enqueued > 0 || !_running; });
                }

                while (!dequeued) {

                    // searches for the first lockable queue containing a task
                    // if no task has been found the loop wraps around
                    // and tries again for the queue identified with the thread index
                    for (size_t n = 0; n < _count + 1; ++n) {
                        if (_queues[(i + n) % _count].try_pop(task)) {
                            dequeued = true;
                            if (_enqueued > 0)
                                _enqueued.fetch_sub(1, std::memory_order_release);

                            task();
                            _active_tasks.fetch_sub(1, std::memory_order_release);

                            // notifies all waiting threads on wait_for_tasks to signal entire batch is finished
                            if (_enqueued.load(std::memory_order_acquire) == 0 &&
                                _active_tasks.load(std::memory_order_acquire) == 0) {
                                std::unique_lock<std::mutex> lock{_mutex};
                                _batch_finished.notify_all();
                            }

                            break;
                        }
                    }

                    if (!(_running.load(std::memory_order_acquire) || _enqueued.load(std::memory_order_acquire) > 0))
                        break;
                }
            }
        }

        //
        //
        //
        //
        //

        const u32                                   _count;
        std::vector<std::thread>                    _threads;
        std::vector<ThreadsafeQueue<Function_type>> _queues;

        std::atomic_size_t _enqueued;
        std::atomic_size_t _active_tasks;
        std::atomic_size_t _index;
        std::atomic_bool   _running;

        std::mutex _mutex;

        std::condition_variable _ready;
        std::condition_variable _batch_finished;
    };


}

#endif //OPENGL_3D_ENGINE_THREAD_POOL_H
