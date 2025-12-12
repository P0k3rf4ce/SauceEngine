#include "utils/Camera.hpp"

#define _USE_MATH_DEFINES
#include <cmath>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#include <Eigen/Geometry>

using namespace Eigen;

float radians(float degrees) {
	return degrees * (M_PI / 180.f);
}

Camera::Camera(Vector3f pos, Vector3f front) {
    this->pos=pos;
    this->up=Vector3f(0.f,1.f,0.f);
    LookAt(front);
}

void Camera::updateView() {

    right=up.cross(front);
	right.normalize();
    view=Camera::lookat(right, up, front, pos);
}

void Camera::LookAt(Vector3f front) {
	this->front=front;
	updateView();
}

void Camera::LookAt(float yaw, float pitch) {
    LookAt(
        Vector3f(
            /* convert euler angles to a direction vector <x,y,z> */
            cos(radians(yaw)) * cos(radians(pitch)),
            sin(radians(pitch)),
            sin(radians(yaw)) * cos(radians(pitch))
        )
    );
}

void Camera::rotate(float radians, Vector3f axis) {
	LookAt(AngleAxisf(radians, axis).toRotationMatrix() * getDirection());
}

Matrix4f Camera::lookat(Vector3f right, Vector3f up, Vector3f direction, Vector3f pos) {
	auto pose = Vector3f(pos(0),pos(1),pos(2));
	return Matrix4f {
		{right(0),     right(1),     right(2),     pose.dot(right)},
		{up(0),        up(1),        up(2),        pose.dot(up)},
		{direction(0), direction(1), direction(2), pose.dot(direction)},
		{0.f,		   0.f,			 0.f,           1.f}
	};
}

void Camera::ProcessKeyboard(Camera::Movement direction, double deltatime) {
    deltatime *= 10.0d;
    switch (direction) {
    case FORWARD:
        pos += deltatime * front;
        break;
    case BACKWARD:
        pos -= deltatime * front;
        break;
    case RIGHT:
        pos -= deltatime * right;
        break;
    case LEFT:
        pos += deltatime * right;
        break;
    }
    updateView();
}

const float SENSITIVITY = 0.1f;

void Camera::ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch)
{
    xoffset *= SENSITIVITY;
    yoffset *= SENSITIVITY;

    Yaw += xoffset;
    Pitch += yoffset;

    // make sure that when pitch is out of bounds, screen doesn't get flipped
    if (constrainPitch)
    {
        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;
    }

    // update Front, Right and Up Vectors using the updated Euler angles
    LookAt(Yaw, Pitch);
}
