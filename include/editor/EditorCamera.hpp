#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace sauce {
class Camera;
class Scene;
}

namespace sauce::editor {

struct Ray {
  glm::vec3 origin;
  glm::vec3 direction;
};

class EditorCamera {
public:
  enum class Mode { Orbit, Fly };

  EditorCamera();

  void update(float deltaTime);
  void syncToSceneCamera(sauce::Camera& camera);

  // Orbit controls
  void orbit(float deltaYaw, float deltaPitch);
  void pan(float deltaX, float deltaY);
  void zoom(float delta);
  void focusOn(const glm::vec3& target);

  // Fly mode controls
  void beginFlyMode();
  void endFlyMode();
  void flyMoveForward(float deltaTime);
  void flyMoveBackward(float deltaTime);
  void flyMoveLeft(float deltaTime);
  void flyMoveRight(float deltaTime);
  void flyMouseLook(float deltaX, float deltaY);

  Mode getMode() const { return mode; }
  glm::vec3 getPosition() const;
  glm::mat4 getViewMatrix() const;
  glm::mat4 getProjectionMatrix(float aspectRatio) const;

  Ray screenToWorldRay(float localX, float localY, float vpWidth, float vpHeight) const;

  glm::vec3 getOrbitTarget() const { return orbitTarget; }
  float getFOV() const { return fov; }

  void setScreenSize(float width, float height) { scrWidth = width; scrHeight = height; }

  void setFlySpeed(float s) { flySpeed = s; }
  void setMouseSensitivity(float s) { mouseSensitivity = s; }
  void setFOV(float f) { fov = f; }

private:
  void updateOrbitPosition();

  Mode mode = Mode::Orbit;

  // Orbit parameters
  glm::vec3 orbitTarget = { 0.0f, 0.0f, 0.0f };
  float orbitDistance = 5.0f;
  float orbitYaw = -90.0f;
  float orbitPitch = 30.0f;
  glm::vec3 orbitPosition;

  // Fly mode parameters
  glm::vec3 flyPosition = { 0.0f, 0.0f, 0.0f };
  float flyYaw = -90.0f;
  float flyPitch = 0.0f;
  glm::vec3 flyFront;
  glm::vec3 flyRight;
  glm::vec3 flyUp;
  float flySpeed = 5.0f;
  float mouseSensitivity = 0.1f;

  float fov = 60.0f;
  float scrWidth = 1920.0f;
  float scrHeight = 1080.0f;

  static constexpr glm::vec3 WORLD_UP = { 0.0f, 0.0f, 1.0f };
};

} // namespace sauce::editor
