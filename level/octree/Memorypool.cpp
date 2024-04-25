//
// Created by Luis Ruisinger on 10.04.24.
//

#include "Memorypool.h"
#include "BoundingVolume.h"

#define INDEX(_p) ((u32) (_p).x + ((_p).z + (_p).y * CHUNK_SIZE) * CHUNK_SIZE)
#define VEC(_i)   ( vec3f {(_i) % CHUNK_SIZE, \
                          ((_i) / CHUNK_SIZE) % CHUNK_SIZE,\
                          ((_i) / CHUNK_SIZE) / CHUNK_SIZE})

template<typename T>
Memorypool<T>::Memorypool(u32 size)
    : capacity{size}
    , memory{std::make_unique<T[]>(this->capacity)}
{
    __builtin_memset(this->memory.get(), 0, this->capacity * sizeof(T));
}

template<typename T>
Memorypool<T>::Memorypool(Memorypool<T> &&other) noexcept
    : capacity{other.capacity}
    , memory{std::move(other.memory)}
{
    other.capacity = 0;
}

//
//
//

template<typename T>
auto Memorypool<T>::operator=(Memorypool<T> &&other) noexcept -> Memorypool<T> & {
    this->capacity = other.capacity;
    this->memory = std::move(other.memory);

    other.capacity = 0;
    return *this;
}

//
//
//

template<typename T>
auto Memorypool<T>::find(vec3f position) const -> std::optional<T *> {
    auto *entry = &this->memory[INDEX(position)];

    return (__builtin_memcmp(entry, this->sentinel, sizeof(T))) ? std::nullopt : std::make_optional(entry);
}

template<typename T>
auto Memorypool<T>::find(u32 index) const -> std::optional<T *> {
    auto *entry = &this->memory.get()[index];

    return (__builtin_memcmp(entry, this->sentinel, sizeof(T))) ? std::nullopt : std::make_optional(entry);
}

//
//
//

template<typename T>
auto Memorypool<T>::insert(vec3f position, T t) -> void {
    this->memory[INDEX(position)] = std::move(t);
}

template<typename T>
auto Memorypool<T>::insert(u32 index, T t) -> void {
    this->memory[index] = std::move(t);
}

//
//
//

template<typename T>
auto Memorypool<T>::remove(vec3f position) -> void {
    __builtin_memset(this->memory.get() + (u64) INDEX(position), 0, sizeof(T));
}

template<typename T>
auto Memorypool<T>::remove(u32 index) -> void {
    __builtin_memset(this->memory.get() + (u64) index, 0, sizeof(T));
}

//
//
//

template<typename T>
auto Memorypool<T>::extract(vec3f position) -> std::optional<T> {
    auto entry = this->memory[INDEX(position)];
    __builtin_memset(this->memory.get() + (u64) INDEX(position), 0, sizeof(T));

    return (__builtin_memcmp(&entry, this->sentinel, sizeof(T))) ? std::nullopt : std::make_optional(entry);
}

template<typename T>
auto Memorypool<T>::extract(u32 index) -> std::optional<T> {
    auto entry = this->memory.get()[index];
    __builtin_memset(this->memory.get() + (u64) index, 0, sizeof(T));

    return (__builtin_memcmp(&entry, this->sentinel, sizeof(T))) ? std::nullopt : std::make_optional(entry);
}

//
//
//

inline
auto indexAsVec(const u32 index) -> vec3f {
    return VEC(index);
}

inline
auto vecAsIndex(const vec3f &vec) -> u32 {
    return INDEX(vec);
}

template class Memorypool<BoundingVolume>;


