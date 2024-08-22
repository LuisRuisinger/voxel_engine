//
// Created by Luis Ruisinger on 04.06.24.
//

#ifndef OPENGL_3D_ENGINE_TICKED_EXECUTOR_H
#define OPENGL_3D_ENGINE_TICKED_EXECUTOR_H

#include <thread>
#include <list>

#include "../util/observable.h"
#include "../camera/camera.h"

namespace core::threading::ticked_executor {

    // default to a tick speed of 20 tps
    template<u32 N = 20>
    class TickedExecutor : util::observable::Observable {
    public:
        TickedExecutor(camera::perspective::Camera &camera)
            : _camera{camera}
        {}

        ~TickedExecutor() override {
            this->_running = false;
            this->_ready.notify_one();
            this->_thread_pool.~Tasksystem();

            if (this->_executor.joinable())
                this->_executor.join();
        }

        auto attach(util::observer::Observer *observer) -> void override {
            this->_observers.push_back(observer);

            if (this->_observer_count.load(std::memory_order_acquire) == 0) {
                ++this->_observer_count;
                this->_ready.notify_one();
            }
            else {
                ++this->_observer_count;
            }
        }

        auto detach(util::observer::Observer *observer) -> void override {
            this->_observers.remove(observer);
            --this->_observer_count;
        }

        auto notfiy() -> void override {
            for (auto &obs : this->_observers)
                obs->tick(this->_thread_pool, this->_camera);
            this->_thread_pool.wait_for_tasks();
        }

        auto run() -> void {
            while (this->_running) {
                {
                    std::unique_lock<std::mutex> lock{this->_mutex};
                    this->_ready.wait(lock, [this] {
                        return this->_observer_count.load(std::memory_order_acquire) > 0;
                    });
                }

                while (this->_observer_count.load(std::memory_order_acquire) > 0 &&
                       this->_running) {
                    auto t_start = std::chrono::high_resolution_clock::now();

                    notfiy();

                    auto t_end  = std::chrono::high_resolution_clock::now();
                    auto t_diff = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start);

                    auto sleep_duration = std::chrono::milliseconds(1000 / N) - t_diff;
                    if (sleep_duration > std::chrono::milliseconds(0))
                        std::this_thread::sleep_for(sleep_duration);
                }
            }
        }

    private:
        threading::Tasksystem<>                _thread_pool {};
        std::list<util::observer::Observer *>  _observers {};
        std::thread                            _executor {[&, this] { run(); }};
        std::mutex                             _mutex {};
        std::atomic_size_t                     _observer_count {0};
        std::condition_variable                _ready {};
        std::atomic_bool                       _running {true};
        camera::perspective::Camera           &_camera;
    };
}

#endif //OPENGL_3D_ENGINE_TICKED_EXECUTOR_H
