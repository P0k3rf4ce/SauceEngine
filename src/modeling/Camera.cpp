#include "modeling/Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(glm::vec3 pos, glm::vec3 front) {
    this->pos=pos;
    this->up=glm::vec3(0.f,1.f,0.f);
    LookAt(front);
}

void Camera::updateView() {
    right=glm::normalize(glm::cross(up, front));
    view=glm::lookAt(pos, pos+front, up);
}

void Camera::LookAt(glm::vec3 front) {
    this->front=front;
    updateView();
}

void Camera::LookAt(float yaw, float pitch) {
    LookAt(
        glm::vec3(
            /* convert euler angles to a direction vector <x,y,z> */
            cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
            sin(glm::radians(pitch)),
            sin(glm::radians(yaw)) * cos(glm::radians(pitch))
        )
    );
}