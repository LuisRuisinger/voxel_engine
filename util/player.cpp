//
// Created by Luis Ruisinger on 25.08.24.
//

#include "player.h"
#include "log.h"

namespace util::player {
    Player::Player(OpenGLKeyMap &key_map, std::shared_ptr<Camera> camera)
            : interactable::Interactable<Player> { key_map },
              camera_hook { std::move(camera) } {
        static_cast<interactable::Interactable<Player> *>(
                this)->template on_input<Action::ON_REPEAT, Keymap::KEY_W>();
        static_cast<interactable::Interactable<Player> *>(
                this)->template on_input<Action::ON_REPEAT, Keymap::KEY_S>();
        static_cast<interactable::Interactable<Player> *>(
                this)->template on_input<Action::ON_REPEAT, Keymap::KEY_A>();
        static_cast<interactable::Interactable<Player> *>(
                this)->template on_input<Action::ON_REPEAT, Keymap::KEY_D>();
        static_cast<interactable::Interactable<Player> *>(
                this)->template on_input<Action::ON_REPEAT, Keymap::KEY_SPACE>();
        static_cast<interactable::Interactable<Player> *>(
                this)->template on_input<Action::ON_REPEAT, Keymap::KEY_C>();
    }

    Player::Player(OpenGLKeyMap &key_map, Camera camera)
            : interactable::Interactable<Player> { key_map },
              camera_hook { std::make_shared<Camera>(std::move(camera)) } {
        static_cast<interactable::Interactable<Player> *>(
                this)->template on_input<Action::ON_REPEAT, Keymap::KEY_W>();
        static_cast<interactable::Interactable<Player> *>(
                this)->template on_input<Action::ON_REPEAT, Keymap::KEY_S>();
        static_cast<interactable::Interactable<Player> *>(
                this)->template on_input<Action::ON_REPEAT, Keymap::KEY_A>();
        static_cast<interactable::Interactable<Player> *>(
                this)->template on_input<Action::ON_REPEAT, Keymap::KEY_D>();
        static_cast<interactable::Interactable<Player> *>(
                this)->template on_input<Action::ON_REPEAT, Keymap::KEY_SPACE>();
        static_cast<interactable::Interactable<Player> *>(
                this)->template on_input<Action::ON_REPEAT, Keymap::KEY_C>();
    }

    auto Player::update_delta_time(f64 dt) -> void {
        this->delta_time = dt;
    }

    auto Player::get_camera() -> Camera & {
        return *this->camera_hook;
    }

    template <>
    auto Player::on_input<Action::ON_REPEAT, Keymap::KEY_W>() -> void {
        camera_hook->ProcessKeyboard(core::camera::FORWARD, this->delta_time);
        DEBUG_LOG(this->camera_hook->getCameraPosition());
    }

    template <>
    auto Player::on_input<Action::ON_REPEAT, Keymap::KEY_S>() -> void {
        camera_hook->ProcessKeyboard(core::camera::BACKWARD, this->delta_time);
        DEBUG_LOG(this->camera_hook->getCameraPosition());
    }

    template <>
    auto Player::on_input<Action::ON_REPEAT, Keymap::KEY_A>() -> void {
        camera_hook->ProcessKeyboard(core::camera::LEFT, this->delta_time);
        DEBUG_LOG(this->camera_hook->getCameraPosition());
    }

    template <>
    auto Player::on_input<Action::ON_REPEAT, Keymap::KEY_D>() -> void {
        camera_hook->ProcessKeyboard(core::camera::RIGHT, this->delta_time);
        DEBUG_LOG(this->camera_hook->getCameraPosition());
    }

    template <>
    auto Player::on_input<Action::ON_REPEAT, Keymap::KEY_SPACE>() -> void {
        camera_hook->ProcessKeyboard(core::camera::UP, this->delta_time);
        DEBUG_LOG(this->camera_hook->getCameraPosition());
    }

    template <>
    auto Player::on_input<Action::ON_REPEAT, Keymap::KEY_C>() -> void {
        camera_hook->ProcessKeyboard(core::camera::DOWN, this->delta_time);
        DEBUG_LOG(this->camera_hook->getCameraPosition());
    }
}


