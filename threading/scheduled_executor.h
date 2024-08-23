//
// Created by Luis Ruisinger on 04.06.24.
//

#ifndef OPENGL_3D_ENGINE_SCHEDULED_EXECUTOR_H
#define OPENGL_3D_ENGINE_SCHEDULED_EXECUTOR_H

#include <thread>
#include <list>

#include "thread.h"

namespace core::threading::executor {

    template<
            u32 TPS = 20,
            typename Function_type = thread::functor::default_function_type>
    class ScheduledExecutor {
    public:
        ScheduledExecutor()
            : executor { [&, this] { run(); }}
        {}

        ~ScheduledExecutor() {
            this->running = false;
            this->ready.notify_one();

            if (this->executor.joinable())
                this->executor.join();
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

            {
                this->task_count.fetch_add(1, std::memory_order_release);
                std::unique_lock lock { this->mutex };
                tasks.push_back(std::move(task));

                if (this->task_count.load(std::memory_order_acquire) == 1)
                    this->ready.notify_one();
            }
        }

        auto run() -> void {
            {
                std::unique_lock lock { this->mutex };
                this->ready.wait(lock, [this] {
                    return this->task_count.load(std::memory_order_acquire) > 0;
                });
            }

            while (this->running) {
                auto t_start = std::chrono::high_resolution_clock::now();

                for (auto &task : this->tasks)
                    task();

                auto t_end  = std::chrono::high_resolution_clock::now();
                auto t_diff = std::chrono::duration_cast<std::chrono::milliseconds>(
                        t_end - t_start);

                auto sleep_duration = std::chrono::milliseconds(1000 / TPS) - t_diff;
                if (sleep_duration > std::chrono::milliseconds(0))
                    std::this_thread::sleep_for(sleep_duration);
            }
        }

    private:
        std::thread executor;
        std::mutex mutex;

        std::atomic<u64> task_count = 0;
        std::atomic<bool> running = true;
        std::condition_variable ready;

        std::vector<Function_type> tasks;
    };
}

#endif //OPENGL_3D_ENGINE_SCHEDULED_EXECUTOR_H
