#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace sauce {

struct CameraCreateInfo {
  float scrWidth;
  float scrHeight;
  glm::vec3 pos = { 2.0f, 2.0f, 2.0f };
  glm::vec3 worldUp = { 0.0f, 0.0f, 1.0f };
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
    this->pos=pos;
    updateView();
  }

  glm::vec3 getPos() const {
    return this->pos;
  }

  /**
   * Sets camera to look at target from position
   */
  void lookAt(const glm::vec3& position, const glm::vec3& target, const glm::vec3& upVec) {
    pos = position;
    worldUp = upVec;
    glm::vec3 direction = glm::normalize(target - position);
    yaw = glm::degrees(atan2(direction.z, direction.x));
    pitch = glm::degrees(asin(direction.y));
    updateView();
  }

  /**
   * Translates the camera position by offset
   *
   * @param offs - offset by which to translate the camera
   */
  void translate(glm::vec3 offs) {
    translate(offs.x, offs.y, offs.z);
  }

  /**
   * Translates the camera position by (x, y, z)
   */
  void translate(float x, float y, float z) {
		this->pos.x+=x;
		this->pos.y+=y;
		this->pos.z+=z;
		updateView();
  }

  /**
   * Get current FOV
   */
  float getFOV() const { return fov; }

  /**
   * Set camera FOV
   */
  void setFOV(float newFov) { this->fov = newFov; }

  void setMovementSpeed(float speed) { this->movementSpeed = speed; }
  float getMovementSpeed() const { return movementSpeed; }

  void setMouseSensitivity(float sensitivity) { this->mouseSensitivity = sensitivity; }
  float getMouseSensitivity() const { return mouseSensitivity; }
  void setScreenSize(float width, float height) {
    scrWidth = width;
    scrHeight = height;
  }

  /**
   * Get view matrix from the current view vectors
   *
   * @return the view matrix for this camera
   */
  glm::mat4 getViewMatrix() const {
		return glm::lookAt(this->pos, this->pos + this->front, this->worldUp);
	}

	/**
	 * Get the projection matrix for this camera
	 *
	 * @return projection matrix for this camera
	 */
	glm::mat4 getProjectionMatrix() const {
		return glm::perspective(glm::radians(this->fov), scrWidth/scrHeight, 0.1f, 100.f);
	}

	/**
	 * Processes direction input, normalized by deltatime and camera velocity
	 *
	 * @param direction - direction in which to move the camera
	 * @param deltatime - the time passed
	 */
	void processDirection(Movement direction, double deltatime) {
		glm::vec3 move;

		switch (direction) {
		case Movement::FORWARD:
			move=this->front;
		    break;
  	    case Movement::BACKWARD:
			move=-1.f*this->front;
		    break;
	    case Movement::LEFT:
			move=-1.f*this->right;
		    break;
	    case Movement::RIGHT:
			move=this->right;
		    break;
	    }
		move*=deltatime*this->movementSpeed;
		this->translate(move);
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
    yaw += xoffset * mouseSensitivity;
		pitch += yoffset * mouseSensitivity;

		if (constrainPitch) {
			pitch = std::max(std::min(pitch, CameraCreateInfo::PITCH_MAX), CameraCreateInfo::PITCH_MIN);
		}

    updateView();
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
    float pitchRad = glm::radians(pitch);
    float yawRad = glm::radians(yaw);

		front.x = cos(pitchRad) * sin(yawRad);
		front.y = cos(pitchRad) * cos(yawRad);
    front.z = sin(pitchRad);

		right=glm::normalize(glm::cross(front, worldUp));
		up=glm::normalize(glm::cross(right, front));
  }
};

}

