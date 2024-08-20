//
// Created by Luis Ruisinger on 29.06.24.
//

#ifndef OPENGL_3D_ENGINE_TAGGED_PTR_H
#define OPENGL_3D_ENGINE_TAGGED_PTR_H

#include "aliases.h"
#include "assert.h"
#include "binary_conversion.h"

#include <stdlib.h>
#include <string>
#include <typeinfo>
#include <iostream>

namespace util::tagged_ptr {

    template <typename _T>
    struct _default_allocator {
        _default_allocator() =default;

        template <typename __T>
        _default_allocator(const _default_allocator<__T> &) {}

        template <typename ...Args>
        INLINE __attribute__((malloc)) static auto alloc(Args &&...args) -> _T * {
            static_assert(sizeof(_T) > 0);
            return new _T { std::forward<Args>(args)... };
        }

        INLINE static auto dealloc(_T *ptr) -> _T * {
            static_assert(sizeof(_T) > 0);
            delete ptr;
        }
    };

    template <typename _T, size_t _N>
    struct _default_allocator<_T[_N]> {
        _default_allocator() =default;

        template<typename __T>
        _default_allocator(const _default_allocator<__T> &) {}

        template <typename ...Args>
        INLINE __attribute__((malloc)) static auto alloc(Args &&...args) -> _T * {
            static_assert(sizeof(_T) > 0);
            return new _T[_N] { std::forward<Args>(args)... };
        }

        INLINE static auto dealloc(_T *ptr) -> void {
            static_assert(sizeof(_T) > 0);
            delete[] ptr;
        }
    };

    template <typename _P, typename StoredType, typename Allocator = _default_allocator<_P>>
    requires (sizeof(StoredType) <= 2) &&
             (!std::is_same_v<_P, void>)
    class TaggedUniquePtr {
        using PointerType = typename std::remove_extent<_P>::type;

    public:

        /**
         * @brief  Constructs a TaggedUniquePtr object by allocating memory on the heap.
         *         The arguments provided (t and args) are forwarded to construct an owned instance of type P.
         * @param  p Pointer to instance of type P.
         * @param  h Optional parameter representing a value of type H, defaults to 0 in bitwise representation.
         */
        INLINE TaggedUniquePtr(
                PointerType *p = nullptr,
                StoredType h = binary_conversion::convert_from_binary<StoredType>(0))
            : ptr { p }
        {
#ifdef DEBUG
            u64 cleared_ptr = reinterpret_cast<u64>(this->ptr) & CLEAR_MASK;
            u64 canonical_ptr = cleared_ptr | (((cleared_ptr & ACCESS_BIT) >> 47) * ONE_MASK);

            ASSERT_EQ(!(canonical_ptr ^ reinterpret_cast<u64>(this->ptr)), "addresses uses more than 48 bits");
#endif
            set_high(h);
        }

        /** @brief Destroys owned instance of P through its pointer. */
        INLINE ~TaggedUniquePtr() { release(); }

        /**
         * @brief Clears object contained in the ptr. Does not canonicalize the ptr.
         */
        INLINE auto clear_high() -> void {
            this->ptr = reinterpret_cast<PointerType *>(reinterpret_cast<u64>(this->ptr) & CLEAR_MASK);
        }

        /**
         * @brief  Returns the contained object in the ptr.
         * @return instance of H.
         */
        [[nodiscard]]
        INLINE auto extract_high() const -> StoredType {
            return binary_conversion::convert_from_binary<StoredType>(
                    static_cast<u16>(reinterpret_cast<u64>(this->ptr) >> 48));
        }

        /**
         * @brief Set the high 16 bit of the ptr to the value an instance of H.
         * @param h The instace of H.
         */
        INLINE auto set_high(StoredType h) -> void {
            u64 val = static_cast<u64>(binary_conversion::convert_to_binary<StoredType>(h)) << 48;
            u64 cleared_ptr = reinterpret_cast<u64>(this->ptr) & CLEAR_MASK;

            this->ptr = reinterpret_cast<PointerType *>(val | cleared_ptr);
        }

        /**
         * @brief  Arrow access overwrite to access canonicalized pointer of P.
         * @return Pointer of P.
         */
        [[nodiscard]]
        INLINE auto operator->() const -> PointerType * {
            u64 cleared_ptr = reinterpret_cast<u64>(this->ptr) & CLEAR_MASK;
            u64 canonical_ptr = cleared_ptr | (((cleared_ptr & ACCESS_BIT) >> 47) * ONE_MASK);

            ASSERT_EQ(canonical_ptr);
            return reinterpret_cast<PointerType *>(canonical_ptr);
        }

        /**
         * @brief  Derenferce overwrite to access through canonicalized pointer of P.
         * @return Moved instance of P.
         */
        [[nodiscard]]
        INLINE auto operator*() const -> PointerType & {
            u64 cleared_ptr = reinterpret_cast<u64>(this->ptr) & CLEAR_MASK;
            u64 canonical_ptr = cleared_ptr | (((cleared_ptr & ACCESS_BIT) >> 47) * ONE_MASK);

            ASSERT_EQ(canonical_ptr);
            return *(reinterpret_cast<PointerType *>(canonical_ptr));
        }

        /**
         * @param  h Instance of h, stora-able in the high 16 bit of the pointer.
         * @return Boolean indicating if the high 16 bit of the pointer equal h.
         */
        [[nodiscard]]
        INLINE auto operator==(StoredType h) const -> bool {
            return (reinterpret_cast<u64>(this->ptr) >> 48) == binary_conversion::convert_to_binary<StoredType>(h);
        }

        /**
         * @param  other Reference to another instance of TaggedUniquePtr.
         * @return Boolean indicating if two TaggedUniquePtr are equal based on the underlying tagged pointer.
         */
        [[nodiscard]]
        INLINE auto operator==(const TaggedUniquePtr &other) const -> bool {
            return reinterpret_cast<u64>(this->ptr) == reinterpret_cast<u64>(other.ptr);
        }

        /**
         * @return Boolean indicating if underlying pointer is canonical,
         *         meaning it can be dereferenced without de-tagging.
         */
        [[nodiscard]]
        INLINE auto canonical() const -> bool {
            u64 r_ptr = reinterpret_cast<u64>(this->ptr);

            return ((((r_ptr & ACCESS_BIT) >> 47) * ONE_MASK) & r_ptr) == 0;
        }

        /**
         * @tparam T Arbitrary type
         * @return Member of type T
         */
        template <typename T>
        [[nodiscard]]
        INLINE auto get() const -> std::conditional<std::is_same_v<T, PointerType>, PointerType *, StoredType> {
            if constexpr (std::is_same_v<T, PointerType>) {
                return operator->();
            }

            if constexpr (std::is_same_v<T, StoredType>) {
                return extract_high();
            }
            else {
                static_assert(
                        std::is_same_v<T, PointerType> || std::is_same_v<T, StoredType>,
                        "Unsupported type");
            }
        }

        [[nodiscard]]
        INLINE auto operator[](size_t i) const -> PointerType & {
            return operator->()[i];
        }

        [[nodiscard]]
        INLINE auto is_null() const -> bool {
            u64 cleared_ptr = reinterpret_cast<u64>(this->ptr) & CLEAR_MASK;
            u64 canonical_ptr = cleared_ptr | (((cleared_ptr & ACCESS_BIT) >> 47) * ONE_MASK);

            return canonical_ptr == 0;
        }

        INLINE auto release() -> void {
            u64 cleared_ptr = reinterpret_cast<u64>(this->ptr) & CLEAR_MASK;
            u64 canonical_ptr = cleared_ptr | (((cleared_ptr & ACCESS_BIT) >> 47) * ONE_MASK);

            // the standard defines that calling delete will nullptr is well-defined and has no effect
            Allocator::dealloc(reinterpret_cast<PointerType *>(canonical_ptr));
        }

        /**
         * ----- Move constructor and assignment -----
         *
         *
         *
         *
         *
         * @brief  Move semantics
         * @tparam _H    Possible other type than the currently stored type.
         * @tparam __P   Possible other type than the current stored pointer to.
         * @param  other Rvalue TaggedUniquePtr instance
         */
        template <typename __P, typename _H>
        INLINE TaggedUniquePtr(TaggedUniquePtr<__P, _H> &&other) noexcept {
            this->ptr = other.ptr;
            other.ptr = nullptr;
        }

        template <typename __P, typename _H>
        INLINE auto operator=(TaggedUniquePtr<__P, _H> &&other) noexcept -> TaggedUniquePtr<__P, _H> & {
            release();

            this->ptr = other.ptr;
            other.ptr = nullptr;

            return *this;
        }

        /**
         * ----- Copy constructor and assignment -----
         *
         *
         *
         *
         *
         * We explicitly want to ensure ownership therefore copying a TaggedPtr
         * or its wrapped pointer shouldn't be possible.
         */
        TaggedUniquePtr(const TaggedUniquePtr &) =delete;
        auto operator=(const TaggedUniquePtr &) -> TaggedUniquePtr & =delete;

    private:

        /** @brief mask to clear high 16 bit of a pointer */
        static const u64 CLEAR_MASK = static_cast<u64>(UINT64_MAX) >> 16;

        /** @brief mask to set high 16 bit of a pointer to all 1's */
        static const u64 ONE_MASK   = static_cast<u64>(UINT16_MAX) << 48;

        /** @brief mask to access the 48th bit, the highest used bit in address translation of a pointer */
        static const u64 ACCESS_BIT = static_cast<u64>(1) << 47;

        /** @brief underlying tagged pointer */
        PointerType *ptr;
    };

    /** @brief Ensures that this pointer type equals the size of a normal pointer */
    static_assert(sizeof(TaggedUniquePtr<uint16_t, uint16_t>) == sizeof(uint16_t *));





    /**
     * ----- Factory to construct tagged Pointers -----
     *
     *
     *
     *
     *
     * @brief  Factory function to construct a TaggedPtr
     * @tparam _P         The type the pointer refers to.
     * @tparam StoredType Type stored in the high 16 bit of the pointer.
     * @tparam Allocator  Allocator used to allocate and deallocate the underlying pointer.
     * @tparam Args       Types of arguments used in the constructor of the underlying pointer.
     * @param  args       Arguments forwarded to construct the owned instance.
     * @param  st         Instance of StoredType defaulted to bit representation of 0.
     * @return Instance of TaggedUniquePtr.
     */
    template <
            typename _P,
            typename StoredType,
            typename Allocator = _default_allocator<_P>,
            typename ...Args>
    [[nodiscard]]
    INLINE static auto make_tagged(
            Args &&...args,
            StoredType st = binary_conversion::convert_from_binary<StoredType>(0))
            -> TaggedUniquePtr<_P, StoredType> {

        // the following will be optimized to just a single in place initialization
        auto tagged_ptr = TaggedUniquePtr<_P, StoredType> {
            Allocator::alloc(std::forward<Args>(args)...),
            binary_conversion::convert_to_binary<StoredType>(st)
        };

        ASSERT_EQ(!tagged_ptr.is_null());
        return tagged_ptr;
    }
}

#endif //OPENGL_3D_ENGINE_TAGGED_PTR_H
