//
// Created by Luis Ruisinger on 29.05.24.
//

#ifndef OPENGL_3D_ENGINE_SINGLETON_H
#define OPENGL_3D_ENGINE_SINGLETON_H

#include <memory>

template<typename T>
class Singleton {
public:
    Singleton(const Singleton &) =delete;
    auto operator=(const Singleton &) -> Singleton & =delete;

    static auto instance() -> T& {
        static T instance{};
        return instance;
    }

protected:
    Singleton() =default;
    ~Singleton() =default;
};

#endif //OPENGL_3D_ENGINE_SINGLETON_H
