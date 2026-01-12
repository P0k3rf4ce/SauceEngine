#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glad/glad.h>

//#include "utils/Logger.hpp"

#define _USE_MATH_DEFINES
#include <cmath>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

class Camera {
public:
    enum class Movement {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT
    };

    static constexpr float YAW_DEFAULT = -90.0f;
    static constexpr float PITCH_DEFAULT = 0.0f;
    static constexpr float SPEED_DEFAULT = 2.5f;
    static constexpr float SENSITIVITY_DEFAULT = 0.1f;
    static constexpr float FOV_DEFAULT = 90.0f;

    void setPos(glm::vec3 pos) {
        this->pos = pos;
        updateView();
    }

    void translate(glm::vec3 offs) {
        this->pos+=offs;
        updateView();
    }
    void translate(float x, float y, float z) {
        pos.x += x; 
        pos.y += y;
        pos.z += z;
        updateView();
    }

    void setFOV(float fov) { this->fov=fov; }

    glm::mat4 getViewMatrix() { return glm::lookAt(pos, pos + front, world_up); }

    glm::mat4 getProjectionMatrix() { 
        return glm::perspective(
            glm::radians(fov), 
            scr_width / scr_height, 
            0.1f, 10.0f
        ); 
    }

    float getFOV() { return fov; }

    Camera(
        float screen_width,
        float screen_height,
        glm::vec3 pos = { 0, 0, 1 },
        glm::vec3 world_up = { 0, 1, 0 },
        float yaw = YAW_DEFAULT,
        float pitch = PITCH_DEFAULT,
        float fov = FOV_DEFAULT
    ) : pos(pos), world_up(world_up), yaw(yaw), pitch(pitch), fov(fov),
    movement_speed(SPEED_DEFAULT), mouse_sensitivity(SENSITIVITY_DEFAULT),
    scr_width(screen_width), scr_height(screen_height) 
    {
        updateView();
    }

    void processKeyboard(Movement direction, double deltatime) {
        switch (direction) {
        case Movement::FORWARD:
            pos += float(movement_speed * deltatime) * front;
            break;
        case Movement::BACKWARD:
            pos -= float(movement_speed * deltatime) * front;
            break;
        case Movement::RIGHT:
            pos += float(movement_speed * deltatime) * right;
            break;
        case Movement::LEFT:
            pos -= float(movement_speed * deltatime) * right;
            break;
        }
        updateView();
    }

    void processMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true) {
        xoffset *= mouse_sensitivity;
        yoffset *= mouse_sensitivity;

        yaw += xoffset;
        pitch += yoffset;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch)
        {
            if (pitch > 89.0f)
                pitch = 89.0f;
            if (pitch < -89.0f)
                pitch = -89.0f;
        }

        updateView();
    }

private:
    glm::vec3 pos;

    /* view orientation vectors */
    glm::vec3 front, up, right;
    
    glm::vec3 world_up;

    float movement_speed, mouse_sensitivity;

    // euler Angles
    float yaw, pitch;

    float fov;
    float scr_width, scr_height;

    /*
    * helper. any time front or pos changes, we need to call
    * this to sync view, right, and up
    */
    void updateView() {
        glm::vec3 new_front;
        new_front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        new_front.y = sin(glm::radians(pitch));
        new_front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        front = glm::normalize(new_front);
        right = glm::normalize(glm::cross(front, world_up));
        up = glm::normalize(glm::cross(right, front));
    }
};

#endif
