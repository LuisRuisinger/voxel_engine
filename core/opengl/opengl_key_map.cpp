//
// Created by Luis Ruisinger on 24.08.24.
//

#include <GLFW/glfw3.h>

#include "opengl_key_map.h"
#include "../../util/log.h"

namespace core::opengl::opengl_key_map {
    OpenGLKeyMap::OpenGLKeyMap() {
        keys[GLFW_KEY_0]               = Keymap::KEY_0;
        keys[GLFW_KEY_1]               = Keymap::KEY_1;
        keys[GLFW_KEY_2]               = Keymap::KEY_2;
        keys[GLFW_KEY_3]               = Keymap::KEY_3;
        keys[GLFW_KEY_4]               = Keymap::KEY_4;
        keys[GLFW_KEY_5]               = Keymap::KEY_5;
        keys[GLFW_KEY_6]               = Keymap::KEY_6;
        keys[GLFW_KEY_7]               = Keymap::KEY_7;
        keys[GLFW_KEY_8]               = Keymap::KEY_8;
        keys[GLFW_KEY_9]               = Keymap::KEY_9;

        keys[GLFW_KEY_A]               = Keymap::KEY_A;
        keys[GLFW_KEY_B]               = Keymap::KEY_B;
        keys[GLFW_KEY_C]               = Keymap::KEY_C;
        keys[GLFW_KEY_D]               = Keymap::KEY_D;
        keys[GLFW_KEY_E]               = Keymap::KEY_E;
        keys[GLFW_KEY_F]               = Keymap::KEY_F;
        keys[GLFW_KEY_G]               = Keymap::KEY_G;
        keys[GLFW_KEY_H]               = Keymap::KEY_H;
        keys[GLFW_KEY_I]               = Keymap::KEY_I;
        keys[GLFW_KEY_J]               = Keymap::KEY_J;
        keys[GLFW_KEY_K]               = Keymap::KEY_K;
        keys[GLFW_KEY_L]               = Keymap::KEY_L;
        keys[GLFW_KEY_M]               = Keymap::KEY_M;
        keys[GLFW_KEY_N]               = Keymap::KEY_N;
        keys[GLFW_KEY_O]               = Keymap::KEY_O;
        keys[GLFW_KEY_P]               = Keymap::KEY_P;
        keys[GLFW_KEY_Q]               = Keymap::KEY_Q;
        keys[GLFW_KEY_R]               = Keymap::KEY_R;
        keys[GLFW_KEY_S]               = Keymap::KEY_S;
        keys[GLFW_KEY_T]               = Keymap::KEY_T;
        keys[GLFW_KEY_U]               = Keymap::KEY_U;
        keys[GLFW_KEY_V]               = Keymap::KEY_V;
        keys[GLFW_KEY_W]               = Keymap::KEY_W;
        keys[GLFW_KEY_X]               = Keymap::KEY_X;
        keys[GLFW_KEY_Y]               = Keymap::KEY_Y;
        keys[GLFW_KEY_Z]               = Keymap::KEY_Z;

        keys[GLFW_KEY_LEFT_BRACKET]    = Keymap::KEY_LEFT_BRACKET;
        keys[GLFW_KEY_RIGHT_BRACKET]   = Keymap::KEY_RIGHT_BRACKET;
        keys[GLFW_KEY_BACKSLASH]       = Keymap::KEY_BACKSLASH;
        keys[GLFW_KEY_GRAVE_ACCENT]    = Keymap::KEY_GRAVE_ACCENT;
        keys[GLFW_KEY_EQUAL]           = Keymap::KEY_EQUAL;
        keys[GLFW_KEY_SLASH]           = Keymap::KEY_SLASH;
        keys[GLFW_KEY_SPACE]           = Keymap::KEY_SPACE;
        keys[GLFW_KEY_APOSTROPHE]      = Keymap::KEY_APOSTROPHE;
        keys[GLFW_KEY_COMMA]           = Keymap::KEY_COMMA;
        keys[GLFW_KEY_MINUS]           = Keymap::KEY_MINUS;
        keys[GLFW_KEY_PERIOD]          = Keymap::KEY_PERIOD;

        keys[GLFW_KEY_ESCAPE]          = Keymap::KEY_ESCAPE;
        keys[GLFW_KEY_ENTER]           = Keymap::KEY_ENTER;
        keys[GLFW_KEY_TAB]             = Keymap::KEY_TAB;
        keys[GLFW_KEY_BACKSPACE]       = Keymap::KEY_BACKSPACE;
        keys[GLFW_KEY_INSERT]          = Keymap::KEY_INSERT;
        keys[GLFW_KEY_DELETE]          = Keymap::KEY_DELETE;
        keys[GLFW_KEY_RIGHT]           = Keymap::KEY_RIGHT;
        keys[GLFW_KEY_LEFT]            = Keymap::KEY_LEFT;
        keys[GLFW_KEY_DOWN]            = Keymap::KEY_DOWN;
        keys[GLFW_KEY_UP]              = Keymap::KEY_UP;
        keys[GLFW_KEY_PAGE_UP]         = Keymap::KEY_PAGE_UP;
        keys[GLFW_KEY_PAGE_DOWN]       = Keymap::KEY_PAGE_DOWN;
        keys[GLFW_KEY_HOME]            = Keymap::KEY_HOME;
        keys[GLFW_KEY_END]             = Keymap::KEY_END;
        keys[GLFW_KEY_CAPS_LOCK]       = Keymap::KEY_CAPS_LOCK;
        keys[GLFW_KEY_SCROLL_LOCK]     = Keymap::KEY_SCROLL_LOCK;
        keys[GLFW_KEY_NUM_LOCK]        = Keymap::KEY_NUM_LOCK;
        keys[GLFW_KEY_PRINT_SCREEN]    = Keymap::KEY_PRINT_SCREEN;
        keys[GLFW_KEY_PAUSE]           = Keymap::KEY_PAUSE;

        keys[GLFW_KEY_F1]              = Keymap::KEY_F1;
        keys[GLFW_KEY_F2]              = Keymap::KEY_F2;
        keys[GLFW_KEY_F3]              = Keymap::KEY_F3;
        keys[GLFW_KEY_F4]              = Keymap::KEY_F4;
        keys[GLFW_KEY_F5]              = Keymap::KEY_F5;
        keys[GLFW_KEY_F6]              = Keymap::KEY_F6;
        keys[GLFW_KEY_F7]              = Keymap::KEY_F7;
        keys[GLFW_KEY_F8]              = Keymap::KEY_F8;
        keys[GLFW_KEY_F9]              = Keymap::KEY_F9;
        keys[GLFW_KEY_F10]             = Keymap::KEY_F10;
        keys[GLFW_KEY_F11]             = Keymap::KEY_F11;
        keys[GLFW_KEY_F12]             = Keymap::KEY_F12;
        keys[GLFW_KEY_F13]             = Keymap::KEY_F13;
        keys[GLFW_KEY_F14]             = Keymap::KEY_F14;
        keys[GLFW_KEY_F15]             = Keymap::KEY_F15;
        keys[GLFW_KEY_F16]             = Keymap::KEY_F16;
        keys[GLFW_KEY_F17]             = Keymap::KEY_F17;
        keys[GLFW_KEY_F18]             = Keymap::KEY_F18;
        keys[GLFW_KEY_F19]             = Keymap::KEY_F19;
        keys[GLFW_KEY_F20]             = Keymap::KEY_F20;
        keys[GLFW_KEY_F21]             = Keymap::KEY_F21;
        keys[GLFW_KEY_F22]             = Keymap::KEY_F22;
        keys[GLFW_KEY_F23]             = Keymap::KEY_F23;
        keys[GLFW_KEY_F24]             = Keymap::KEY_F24;
        keys[GLFW_KEY_F25]             = Keymap::KEY_F25;
        keys[GLFW_KEY_LEFT_SHIFT]      = Keymap::LEFT_SHIFT;
        keys[GLFW_KEY_LEFT_ALT]        = Keymap::LEFT_ALT;
        keys[GLFW_KEY_LEFT_CONTROL]    = Keymap::LEFT_CONTROL;
        keys[GLFW_KEY_RIGHT_SHIFT]     = Keymap::RIGHT_SHIFT;
        keys[GLFW_KEY_RIGHT_ALT]       = Keymap::RIGHT_ALT;
        keys[GLFW_KEY_RIGHT_CONTROL]   = Keymap::RIGHT_CONTROL;
    }



    auto OpenGLKeyMap::remove_callback(Action action, Keymap key) -> void {
        switch (action) {
            case ON_PRESSED: this->on_pressed.erase(key); break;
            case ON_RELEASE: this->on_release.erase(key); break;
            case ON_REPEAT : this->on_repeat.erase(key);  break;
        }
    }

    auto OpenGLKeyMap::handle_event(std::pair<i32, i32> &ref) -> void {
        if (!keys.contains(ref.second))
            return;

        Keymap key = keys[ref.second];
        switch (ref.first) {
            case ON_PRESSED: {
                if (this->on_pressed.contains(key)) {
                    if (this->on_repeat.contains(key)) {
                        this->repeat_functions[key] = this->on_pressed[key];
                    }
                    else {
                        this->on_pressed[key]();
                    }
                }
                break;
            }
            case ON_RELEASE: {
                if (this->repeat_functions.contains(key)) {
                    this->repeat_functions.erase(key);
                }

                if (this->on_release.contains(key)) {
                    this->on_release[key]();
                }
                break;
            }

            // currently no concept for using this
            case ON_REPEAT: {}
        }
    }

    auto OpenGLKeyMap::run_repeat() -> void {
        for (auto &[_, callback] : this->repeat_functions)
            callback();
    }
}
