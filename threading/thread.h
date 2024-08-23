//
// Created by Luis Ruisinger on 23.08.24.
//

#ifndef OPENGL_3D_ENGINE_THREAD_H
#define OPENGL_3D_ENGINE_THREAD_H

#include <functional>

#define WRAPPED_EXEC(_f) ({      \
    try {                        \
        (_f);                    \
    }                            \
    catch(std::exception &err) { \
        LOG(err.what()) ;        \
    }})

namespace core::threading::thread {
    namespace functor {
#ifdef __cpp_lib_move_only_function
        using default_function_type = std::move_only_function<void()>;
#else
        using default_function_type = std::function<void()>;
#endif
    }
}

#endif //OPENGL_3D_ENGINE_THREAD_H
