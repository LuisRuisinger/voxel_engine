//
// Created by Luis Ruisinger on 07.07.24.
//

#ifndef OPENGL_3D_ENGINE_REFLECTIONS_H
#define OPENGL_3D_ENGINE_REFLECTIONS_H

#include <tuple>
#include <type_traits>

namespace util::reflections {
    template <typename Type, typename MemberFunctionName, typename = void>
    struct has_member : std::false_type {};

    template <typename Type, typename ReturnType, typename ...Args>
    struct has_member<Type, ReturnType(Type::*)(Args...)> : std::true_type {};

    template <typename Type, typename ReturnType, typename ...Args>
    struct has_member<Type, ReturnType(Type::*)(Args...) const> : std::true_type {};

    template <typename Type, typename ReturnType, typename ...Args>
    inline constexpr bool has_member_v = has_member<Type, ReturnType, Args...>::value;

    template <typename T, typename Tuple>
    struct has_type;

    template <typename T, typename ...Args>
    struct has_type<T, std::tuple<Args...>> : std::disjunction<std::is_same<T, Args>...> {};

    template <typename T, typename ...Args>
    inline constexpr bool has_type_v = has_type<T, Args...>::value;

    template <typename ...Args>
    constexpr auto tuple_from_params(Args ...args) -> std::tuple<Args ...> {
        return std::tuple<
                typename std::add_lvalue_reference<
                        typename std::add_const<Args>::type>::type...> {
                            std::forward<Args>(args)...
                        };
    }

}

#endif //OPENGL_3D_ENGINE_REFLECTIONS_H
