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

    Player::Player(OpenGLKeyMap &key_map)
            : interactable::Interactable<Player> { key_map },
              camera_hook { std::make_shared<Camera>() } {
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

    auto Player::tick(core::state::State &state) -> void {
        this->delta_time = state.delta_time;
        this->intersection =
                aabb_ray_intersection::Ray(
                        this->camera_hook->get_position(),
                        this->camera_hook->get_front()).intersect(state.platform);

        if (this->intersection) {
            const auto &ref = this->intersection.value();
        }
    }

    auto Player::get_camera() -> Camera & {
        return *this->camera_hook;
    }

    template <>
    auto Player::on_input<Action::ON_REPEAT, Keymap::KEY_W>() -> void {
        camera_hook->move_camera(util::camera::FORWARD, this->delta_time);
    }

    template <>
    auto Player::on_input<Action::ON_REPEAT, Keymap::KEY_S>() -> void {
        camera_hook->move_camera(util::camera::BACKWARD, this->delta_time);
    }

    template <>
    auto Player::on_input<Action::ON_REPEAT, Keymap::KEY_A>() -> void {
        camera_hook->move_camera(util::camera::LEFT, this->delta_time);
    }

    template <>
    auto Player::on_input<Action::ON_REPEAT, Keymap::KEY_D>() -> void {
        camera_hook->move_camera(util::camera::RIGHT, this->delta_time);
    }

    template <>
    auto Player::on_input<Action::ON_REPEAT, Keymap::KEY_SPACE>() -> void {
        camera_hook->move_camera(util::camera::UP, this->delta_time);
    }

    template <>
    auto Player::on_input<Action::ON_REPEAT, Keymap::KEY_C>() -> void {
        camera_hook->move_camera(util::camera::DOWN, this->delta_time);
        DEBUG_LOG(this->camera_hook->get_position());
    }
}


