//
// Created by Luis Ruisinger on 15.02.24.
//

#ifndef OPENGL_3D_ENGINE_CAMERA_H
#define OPENGL_3D_ENGINE_CAMERA_H

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glad/glad.h"

#include "defines.h"
#include "culling.h"

#define YAW         0.0F
#define PITCH       0.0F
#define SPEED       32.0F
#define SENSITIVITY 0.1F
#define MAX_PITCH   89.0F
#define MIN_PITCH   -89.0F
#define DEFAULT_FOV 75.0F

namespace util::camera {
    constexpr const f32 near_plane = 0.1F;

    enum Camera_Movement : u8 {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
        UP,
        DOWN
    };

    class Camera {
    public:
        Camera(glm::vec3, glm::vec3, f32, f32);
        Camera();

        Camera(const Camera &) =delete;
        Camera(Camera &&) noexcept =default;

        ~Camera() =default;

        auto operator=(const Camera &) -> Camera & =delete;
        auto operator=(Camera &&) noexcept -> Camera & =default;

        auto init() -> void;

        auto move_camera(Camera_Movement, f32) -> void;
        auto rotate_camera(f32, f32) -> void;

        auto set_pitch(f32) -> void;
        auto increase_pitch(f32) -> void;

        auto set_position(glm::vec3) -> void;
        auto increase_position(glm::vec3) -> void;

        auto set_frustum(f32, f32, f32, f32) -> void;
        auto set_frustum_aspect(f32) -> void;
        auto set_frustum_definition(glm::vec3, glm::vec3, glm::vec3) -> void;

        auto check_in_frustum(glm::vec3, u32) const -> bool;
        auto check_in_frustum(glm::vec2, u32) const -> bool;
        auto frustum_collision(glm::vec3, u32 ) const -> culling::CollisionType;
        auto frustum_collision(glm::vec2, u32) const -> culling::CollisionType;

        auto get_position() const -> glm::vec3;
        auto get_front() const ->  glm::vec3;
        auto get_mask() const -> u8;
        auto get_view_matrix() const -> const glm::mat4 &;
        auto update() -> void;

        auto set_projection_matrix(i32, i32) -> void;
        auto get_projection_matrix() -> const glm::mat4 &;

    private:

        // camera 3d world space attributes
        glm::vec3 position;
        glm::vec3 front;
        glm::vec3 up;
        glm::vec3 right;
        glm::vec3 world_up;

        // matrices
        glm::mat4 view_matrix;
        glm::mat4 projection_matrix;

        // angles and 2d plane position
        f32 yaw;
        f32 pitch;
        f32 last_xpos;
        f32 last_ypos;

        // options
        f32 movement_speed;
        f32 mouse_sensitifity;

        // culling
        culling::Frustum frustum;
        u8 aa_visible_face_mask;
    };
}


#endif //OPENGL_3D_ENGINE_CAMERA_H
