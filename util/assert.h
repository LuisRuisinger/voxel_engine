//
// Created by Luis Ruisinger on 14.06.24.
//

#ifndef OPENGL_3D_ENGINE_ASSERT_H
#define OPENGL_3D_ENGINE_ASSERT_H

#include "log.h"

#include <stdlib.h>

#ifdef DEBUG
    #define ASSERT(_x, _m)                                                                                          \
        {                                                                                                           \
        static_assert(std::is_convertible_v<decltype((_x)), bool>);                                                 \
        static_assert(std::is_convertible_v<decltype((_m)), std::string>);                                          \
        if (!(_x)) {                                                                                                \
            util::log::out() << util::log::Level::LOG_LEVEL_ERROR                                                   \
                             << "ASSERTION FAILED" + (std::string(_m).size() > 0 ? (": " + std::string(_m)) : "")   \
                             << util::log::end;                                                                     \
            std::exit(EXIT_FAILURE);                                                                                \
        }                                                                                                           \
        }
#else
    #define ASSERT(_x, _m)
#endif
#endif //OPENGL_3D_ENGINE_ASSERT_H
