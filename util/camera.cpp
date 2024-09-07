//
// Created by Luis Ruisinger on 15.02.24.
//

#include <algorithm>

#include "camera.h"
#include "aliases.h"

#define HORIZONTAL_THRESHOLD 0.55F
#define VERTICAL_THRESHOLD   0.25F

namespace util::camera {
    Camera::Camera(glm::vec3 position, glm::vec3 up , f32 yaw, f32 pitch)
            : position             { position    },
              world_up             { up          },
              yaw                  { yaw         },
              pitch                { pitch       },
              movement_speed       { SPEED       },
              mouse_sensitifity    { SENSITIVITY },
              aa_visible_face_mask { UINT8_MAX   } {
        init();
    }

    Camera::Camera()
            : position             { 0.0f, 2.5f, 0.0f },
              world_up             { 0.0f, 1.0f, 0.0f },
              yaw                  { YAW              },
              pitch                { PITCH            },
              movement_speed       { SPEED            },
              mouse_sensitifity    { SENSITIVITY      },
              aa_visible_face_mask { UINT8_MAX        } {
        init();
    }

    auto Camera::init() -> void {
        set_frustum(
                DEFAULT_FOV,
                static_cast<f32>(DEFAULT_WIDTH) / static_cast<f32>(DEFAULT_HEIGHT),
                0.1F,
                CHUNK_SIZE * (RENDER_RADIUS * 2));
        update();
        set_projection_matrix(DEFAULT_WIDTH, DEFAULT_HEIGHT);
    }

    void Camera::move_camera(Camera_Movement direction, float delta_time) {
        f32 velocity = this->movement_speed * delta_time;

        switch (direction) {
            case FORWARD : this->position += this->front * velocity; break;
            case BACKWARD: this->position -= this->front * velocity; break;
            case LEFT    : this->position -= this->right * velocity; break;
            case RIGHT   : this->position += this->right * velocity; break;
            case UP      : this->position += this->up * velocity;    break;
            case DOWN    : this->position -= this->up * velocity;    break;
        }

        update();
    }

    auto Camera::rotate_camera(f32 xpos, f32 ypos) -> void {
        f32 x_offset = xpos - this->last_xpos;
        f32 y_offset = this->last_ypos - ypos;

        this->last_xpos = xpos;
        this->last_ypos = ypos;

        this->yaw += x_offset * this->mouse_sensitifity;
        this->pitch += y_offset * this->mouse_sensitifity;

        this->pitch = std::min(MAX_PITCH, this->pitch);
        this->pitch = std::max(MIN_PITCH, this->pitch);
        update();
    }

    auto Camera::update() -> void {
        f32 cos_yaw = cos(glm::radians(this->yaw));
        f32 cos_pitch = cos(glm::radians(this->pitch));

        this->front = glm::vec3 {
                cos_yaw * cos_pitch,
                sin(glm::radians(this->pitch)),
                sin(glm::radians(this->yaw)) * cos_pitch
        };

        this->front = glm::normalize(this->front);
        this->right = glm::normalize(glm::cross(this->front, this->world_up));
        this->up = glm::normalize(glm::cross(this->right, this->front));
        this->view_matrix = glm::lookAt(
                this->position,
                this->position + this->front,
                this->up);

        set_frustum_definition(this->position, this->front + this->position, this->up);

        this->aa_visible_face_mask = UINT8_MAX;
        this->aa_visible_face_mask ^= (this->front.x >  HORIZONTAL_THRESHOLD) * (RIGHT_BIT  >> 10);
        this->aa_visible_face_mask ^= (this->front.x < -HORIZONTAL_THRESHOLD) * (LEFT_BIT   >> 10);
        this->aa_visible_face_mask ^= (this->front.y >  VERTICAL_THRESHOLD)   * (TOP_BIT    >> 10);
        this->aa_visible_face_mask ^= (this->front.y < -VERTICAL_THRESHOLD)   * (BOTTOM_BIT >> 10);
        this->aa_visible_face_mask ^= (this->front.z >  HORIZONTAL_THRESHOLD) * (FRONT_BIT  >> 10);
        this->aa_visible_face_mask ^= (this->front.z < -HORIZONTAL_THRESHOLD) * (BACK_BIT   >> 10);
    }

    auto Camera::set_frustum(f32 angle, f32 ratio, f32 nearD, f32 farD) -> void {
        this->frustum.set_cam_internals(angle, ratio, nearD, farD);
    }

    auto Camera::set_frustum_aspect(f32 r) -> void {
        this->frustum.set_cam_internals(DEFAULT_FOV, r, 0.1F, CHUNK_SIZE * (RENDER_RADIUS * 2));
    }

    auto Camera::set_frustum_definition(glm::vec3 p, glm::vec3 t, glm::vec3 u) -> void {
        this->frustum.set_cam_definition(p, t, u);
    }

    auto Camera::check_in_frustum(glm::vec2 p, u32 s) const -> bool {
        return this->frustum.square_visible(p, s);
    }

    auto Camera::check_in_frustum(glm::vec3 p, u32 s) const -> bool {
        return this->frustum.cube_visible(p, s);
    }

    auto Camera::frustum_collision(glm::vec2 p, u32 s) const -> culling::CollisionType {
        return this->frustum.squere_visible_type(p, s);
    }

    auto Camera::frustum_collision(glm::vec3 p, u32 s) const -> culling::CollisionType {
        return this->frustum.cube_visible_type(p, s);
    }

    auto Camera::get_position() const -> glm::vec3 {
        return this->position;
    }

    auto Camera::get_front() const -> glm::vec3 {
        return this->front;
    }

    auto Camera::get_mask() const -> u8 {
        return this->aa_visible_face_mask;
    }

    auto Camera::get_view_matrix() const -> const glm::mat4 & {
        return this->view_matrix;
    }

    auto Camera::set_projection_matrix(i32 width, i32 height) -> void {
        this->projection_matrix =
                glm::perspective(
                        glm::radians(60.0F),
                        static_cast<f32>(width) / static_cast<f32>(height),
                        0.1F,
                        (static_cast<f32>(RENDER_RADIUS) + 4.0F) * static_cast<f32>(CHUNK_SIZE));
    }

    auto Camera::get_projection_matrix() -> const glm::mat4 & {
        return this->projection_matrix;
    }
}
