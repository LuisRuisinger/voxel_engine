//
// Created by Luis Ruisinger on 10.04.24.
//

#ifndef OPENGL_3D_ENGINE_MEMORYPOOL_H
#define OPENGL_3D_ENGINE_MEMORYPOOL_H

#include <cstdlib>
#include <memory>
#include <optional>
#include <ranges>

#include "../../util/aliases.h"

template<typename T>
class Memorypool {
public:
    Memorypool(u32);
    ~Memorypool() = default;

    Memorypool(Memorypool<T> &) = delete;
    auto operator=(Memorypool<T> &) noexcept -> Memorypool<T> & = delete;

    Memorypool(Memorypool<T> &&) noexcept;
    auto operator=(Memorypool<T> &&) noexcept -> Memorypool<T> &;

    auto insert(vec3f, T) -> void;
    auto insert(u32, T) -> void;

    auto remove(vec3f) -> void;
    auto remove(u32) -> void;

    auto extract(vec3f) -> std::optional<T>;
    auto extract(u32) -> std::optional<T>;


    auto find(vec3f) const -> std::optional<T *>;
    auto find(u32) const -> std::optional<T *>;

private:
    const u8 sentinel[sizeof(T)] = {0};
    u32 capacity;

    std::unique_ptr<T[]> memory;
};

inline
auto indexAsVec(const u32) -> vec3f;

inline
auto vecAsIndex(const vec3f) -> u32;

#endif //OPENGL_3D_ENGINE_MEMORYPOOL_H
