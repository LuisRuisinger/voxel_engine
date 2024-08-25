//
// Created by Luis Ruisinger on 25.08.24.
//

#ifndef OPENGL_3D_ENGINE_PLAYER_H
#define OPENGL_3D_ENGINE_PLAYER_H

#include "interactable.h"
#include "../camera/camera.h"

namespace util::player {
    using namespace core::opengl::opengl_key_map;
    using namespace core::camera::perspective;

    class Player : public interactable::Interactable<Player> {
    public:
        Player(OpenGLKeyMap &key_map, std::shared_ptr<Camera> camera);
        Player(OpenGLKeyMap &key_map, Camera camera);


        template <Action action, Keymap key>
        auto on_input() -> void;
        auto get_camera() -> Camera &;

        auto update_delta_time(f64) -> void;

    private:
        std::shared_ptr<Camera> camera_hook;
        f64 delta_time;
    };
}

#endif //OPENGL_3D_ENGINE_PLAYER_H
