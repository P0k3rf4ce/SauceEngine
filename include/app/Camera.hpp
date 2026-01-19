#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace sauce {

struct CameraCreateInfo {
  float scrWidth;
  float scrHeight;
  glm::vec3 pos = { 0, 0, 1 };
  glm::vec3 worldUp = { 0, 1, 0 };
  float yaw = YAW_DEFAULT;
  float pitch = PITCH_DEFAULT;
  float fov = FOV_DEFAULT;
  float movementSpeed = SPEED_DEFAULT;
  float mouseSensitivity = SENSITIVITY_DEFAULT;

  static constexpr float YAW_DEFAULT = -90.0f;
  static constexpr float PITCH_DEFAULT = 0.0f;
  static constexpr float SPEED_DEFAULT = 2.5f;
  static constexpr float SENSITIVITY_DEFAULT = 0.1f;
  static constexpr float FOV_DEFAULT = 90.0f;

  static constexpr float PITCH_MIN = -89.0f;
  static constexpr float PITCH_MAX = 89.0f;
};

class Camera {
public:
    enum class Movement {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT
    };

    /**
     * Camera constructor
     */
    Camera(const sauce::CameraCreateInfo& createInfo)
      : pos(createInfo.pos), worldUp(createInfo.worldUp), 
      yaw(createInfo.yaw), pitch(createInfo.pitch), fov(createInfo.fov),
      movementSpeed(createInfo.movementSpeed), mouseSensitivity(createInfo.mouseSensitivity),
      scrWidth(createInfo.scrWidth), scrHeight(createInfo.scrHeight)
    {
        updateView();
    }

    /**
     * Sets camera position
     *
     * @param pos - position to move the camera to
     */
    void setPos(glm::vec3 pos) {
      // TODO
    }

    /**
     * Translates the camera position by offset 
     *
     * @param offs - offset by which to translate the camera
     */
    void translate(glm::vec3 offs) {
      // TODO 
    }

    /**
     * Translates the camera position by (x, y, z)
     */
    void translate(float x, float y, float z) {
      // TODO
    }

    /**
     * Get current FOV
     */
    float getFOV() const { return fov; }

    /**
     * Set camera FOV
     */
    void setFOV(float fov) { this->fov=fov; }

    /**
     * Get view matrix from the current view vectors
     *
     * @return the view matrix for this camera
     */
    glm::mat4 getViewMatrix() const {
      // TODO (Use glm::lookAt)
      return glm::mat4(1.0f);
    }

    /**
     * Get the projection matrix for this camera
     *
     * @return projection matrix for this camera
     */
    glm::mat4 getProjectionMatrix() const {
      // TODO (use glm::perspective)
      return glm::mat4(1.0f);
    }

    /**
     * Processes direction input, normalized by deltatime and camera velocity
     *
     * @param direction - direction in which to move the camera 
     * @param deltatime - the time passed
     */
    void processDirection(Movement direction, double deltatime) {
      // TODO 
    }

    /**
     * Processes mouse movements by offsetting yaw and pitch by xoffset and yoffset, resp.
     * If constrainPitch is true, pitch gets clamped to PITCH_MIN or PITCH_MAX when out of bounds.
     *
     * @param xoffset - offset for yaw 
     * @param yoffset - offset for pitch 
     * @param constrainPitch - whether or not to clamp pitch when out of bounds
     */
    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true) {
      // TODO
    }

private:
    glm::vec3 pos;

    /* view orientation vectors */
    glm::vec3 front, up, right;
    
    /* Used to get the right vector from front vector */
    glm::vec3 worldUp;

    float movementSpeed, mouseSensitivity;

    // euler Angles
    float yaw, pitch;

    float fov;
    float scrWidth, scrHeight;

    /**
     * Sets the front, right, and up vectors using the current values for yaw and pitch.
     * This should be called any time the camera position or angle is changed.
     */
    void updateView() {
      // TODO 
    }
};

}

