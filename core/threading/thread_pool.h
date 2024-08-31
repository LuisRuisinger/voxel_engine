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
#include <random>

#include "thread_safe_queue.h"
#include "spmc_queue.h"
#include "../../util/aliases.h"
#include "../../util/log.h"
#include "../../util/assert.h"
#include "thread.h"

namespace core::threading::thread_pool {
    inline thread_local u64 worker_id;

    template<typename Function_type = thread::functor::default_function_type>
    requires std::invocable<Function_type> &&
             std::is_same_v<void, std::invoke_result_t<Function_type>>
    class Tasksystem {
    public:

        /**
         * @brief Thread pool constructor.
         * @param thread_count Amount of threads this instance should hold.
         *                     Defaults number of concurrent threads supported by the implementation.
         * @post  Threads are initialized and running. Waiting for work being available in the queue.
         */
        explicit Tasksystem(u32 thread_count = std::thread::hardware_concurrency())
                : thread_instance_count{thread_count},
                  task_queue{thread_count}
        {
            for (size_t i = 0; i < this->thread_instance_count; ++i)
                this->thread_instances.emplace_back(
                        [&, idx = i] {
                            run(idx);
                        });
        }

        /** @brief Thread pool destructor, joining all threads. */
        ~Tasksystem() {
            this->worker_runflag = false;
            this->tasks_available.notify_all();

            for (auto &t : this->thread_instances)
                if (t.joinable()) t.join();
        }

        /**
         * @brief  Try to schedule a task into the thread pool.
         * @tparam F An invokable type.
         * @param  f The to be executed callable.
         */
        template <typename F>
        auto try_schedule(F &&f) -> bool {
            u32 i = this->enqueue_task_index.fetch_add(
                    1,
                    std::memory_order_release) % this->thread_instance_count;

            for (size_t n = 0; n < this->thread_instance_count; ++n) {
                if (this->task_queue[(i + n) % this->thread_instance_count].try_push(
                        std::forward<F>(f))) {
                    this->enqueued_tasks_count.fetch_add(1, std::memory_order_relaxed);
                    this->active_tasks_count.fetch_add(1, std::memory_order_relaxed);
                    this->tasks_available.notify_one();

                    return true;
                }
            }

            return false;
        }

        /**
         * @brief  Enqueue a task that returns void.
         * @tparam F An invokable type.
         * @tparam Args Argument pack for F.
         * @param  fun The to be executed callable.
         * @param  args Arguments that will be passed to fun.
         */
        template<typename F, typename ...Args>
        requires std::invocable<F, Args...> &&
                 std::is_same_v<void, std::invoke_result_t<F &&, Args &&...>>
        auto enqueue_detach(F &&fun, Args &&...args) -> void {
            auto task = [
                    fun = std::forward<F>(fun),
                    ...args = std::forward<Args>(args)]() mutable -> decltype(auto) {
                    WRAPPED_EXEC(std::invoke(fun, args...));
            };

            // trying until enqueued
            while (!try_schedule(std::move(task)));
        }

        /**
         * @brief  Enqueue a task with async result.
         * @tparam F An invokable type.
         * @tparam Args Argument pack for F.
         * @param  fun The to be executed callable.
         * @param  args Arguments that will be passed to fun.
         * @return A future of the result of fun.
         */
        template<
                typename F,
                typename ...Args ,
                typename Ret = std::invoke_result_t<F &&, Args &&...>>
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
            auto packaged_task = std::packaged_task<Ret>(
                    std::bind(std::forward<F>(fun),std::forward<Args>(args)...));

            auto promise = std::make_shared<std::promise<Ret>>(packaged_task.getfuture());
            auto future = promise->get_future();

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
            // trying until enqueued
            while (!try_schedule(std::move(task)));
            return future;
        }

        /**
         * @brief Calling thread waits until every queued task has been finished.
         * @param timeout Timeout after which the calling thread returns. Defaulted to 5ms.
         *                If set to 0 no timeout will be applied.
         */
        auto wait_for_tasks() -> void {
            /*
            std::unique_lock lock { this->mutex };
            this->tasks_finished.wait(lock, [this] {
                return no_tasks();
            });
             */

            while (!no_tasks())
                std::this_thread::yield();
        }

        /**
         * @brief  Check if all tasks have been finished and no unscheduled work remains.
         *         Non-blocking.
         * @return Boolean value to indicate if batch is finished.
         */
        inline auto no_tasks() -> bool {
            return this->enqueued_tasks_count.load(std::memory_order_acquire) < 1 &&
                   this->active_tasks_count.load(std::memory_order_acquire) < 1;
        }

        Tasksystem(Tasksystem &&) noexcept =default;
        auto operator=(Tasksystem &&) noexcept -> Tasksystem & =default;

        Tasksystem(const Tasksystem &) =delete;
        auto operator=(const Tasksystem &) -> Tasksystem & =delete;

    private:

        /**
         * @brief Runnable wrapper function for each worker.
         * @param i The index of the thread inside the task system.
         */
        auto run(u32 i) -> void {
            DEBUG_LOG("Thread id " + std::to_string(i) + " init");
            worker_id = i;

            Function_type task;
            bool dequeue;

            while (this->worker_runflag.load(std::memory_order_relaxed)) {
                {
                    std::unique_lock<std::mutex> lock { this->mutex };
                    this->tasks_available.wait(lock, [this] {
                        return this->enqueued_tasks_count.load(std::memory_order_acquire) > 0 ||
                               !this->worker_runflag.load(std::memory_order_acquire);
                    });
                }

                for (;;) {

                    // searches for the first lockable queue containing a task
                    // if no task has been found the loop wraps around
                    // and tries again for the queue identified with the thread index
                    for (size_t n = 0; n < this->thread_instance_count + 1; ++n) {
                        if (this->task_queue[(i + n) %
                            this->thread_instance_count].try_pop(task)) {
                            this->enqueued_tasks_count.fetch_sub(1, std::memory_order_relaxed);

                            // through the relaxed subtraction of the enqueued count it could
                            // be possible a thread tries to extract a non-valid element
                            // of the SPMC queue
                            if (task)
                                task();

                            this->active_tasks_count.fetch_sub(1, std::memory_order_relaxed);
                        }
                    }

                    if (this->enqueued_tasks_count.load(std::memory_order_acquire) < 1) {
                        if (this->active_tasks_count.load(std::memory_order_acquire) < 1)
                            this->tasks_finished.notify_all();

                        break;
                    }
                }
            }
        }

        const u32                                   thread_instance_count;
        std::vector<std::thread>                    thread_instances;
        // std::vector<ThreadsafeQueue<Function_type>> task_queue;
        std::vector<spmc_queue::SPMCQueue<Function_type>> task_queue;

        std::atomic_size_t enqueued_tasks_count = 0;
        std::atomic_size_t active_tasks_count = 0;
        std::atomic_size_t enqueue_task_index = 0;
        std::atomic_bool   worker_runflag = true;

        std::mutex mutex;

        std::condition_variable tasks_available;
        std::condition_variable tasks_finished;
    };


}

#endif //OPENGL_3D_ENGINE_THREAD_POOL_H
