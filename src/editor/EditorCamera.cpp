#include <editor/EditorCamera.hpp>
#include <app/Camera.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cmath>

namespace sauce::editor {

EditorCamera::EditorCamera() {
  updateOrbitPosition();
}

void EditorCamera::updateOrbitPosition() {
  float pitchRad = glm::radians(orbitPitch);
  float yawRad = glm::radians(orbitYaw);

  orbitPosition.x = orbitTarget.x + orbitDistance * cos(pitchRad) * sin(yawRad);
  orbitPosition.y = orbitTarget.y + orbitDistance * cos(pitchRad) * cos(yawRad);
  orbitPosition.z = orbitTarget.z + orbitDistance * sin(pitchRad);
}

void EditorCamera::update(float /*deltaTime*/) {
  if (mode == Mode::Orbit) {
    updateOrbitPosition();
  } else {
    // Update fly vectors from yaw/pitch
    float pitchRad = glm::radians(flyPitch);
    float yawRad = glm::radians(flyYaw);

    flyFront.x = cos(pitchRad) * sin(yawRad);
    flyFront.y = cos(pitchRad) * cos(yawRad);
    flyFront.z = sin(pitchRad);
    flyFront = glm::normalize(flyFront);

    flyRight = glm::normalize(glm::cross(flyFront, WORLD_UP));
    flyUp = glm::normalize(glm::cross(flyRight, flyFront));
  }
}

void EditorCamera::syncToSceneCamera(sauce::Camera& cam) {
  glm::vec3 pos = getPosition();

  glm::vec3 dir;
  if (mode == Mode::Orbit) {
    dir = glm::normalize(orbitTarget - orbitPosition);
  } else {
    dir = flyFront;
  }

  cam.lookAt(pos, pos + dir, WORLD_UP);
}

void EditorCamera::orbit(float deltaYaw, float deltaPitch) {
  orbitYaw += deltaYaw;
  orbitPitch += deltaPitch;
  orbitPitch = glm::clamp(orbitPitch, -89.0f, 89.0f);
  updateOrbitPosition();
}

void EditorCamera::pan(float deltaX, float deltaY) {
  glm::vec3 forward = glm::normalize(orbitTarget - orbitPosition);
  glm::vec3 right = glm::normalize(glm::cross(forward, WORLD_UP));
  glm::vec3 up = glm::normalize(glm::cross(right, forward));

  float panSpeed = orbitDistance * 0.002f;
  orbitTarget += right * (-deltaX * panSpeed) + up * (deltaY * panSpeed);
  updateOrbitPosition();
}

void EditorCamera::zoom(float delta) {
  orbitDistance -= delta * orbitDistance * 0.1f;
  orbitDistance = glm::clamp(orbitDistance, 0.1f, 500.0f);
  updateOrbitPosition();
}

void EditorCamera::focusOn(const glm::vec3& target) {
  orbitTarget = target;
  orbitDistance = 5.0f;
  updateOrbitPosition();
}

void EditorCamera::beginFlyMode() {
  mode = Mode::Fly;
  flyPosition = orbitPosition;

  glm::vec3 dir = glm::normalize(orbitTarget - orbitPosition);
  flyPitch = glm::degrees(asin(glm::clamp(dir.z, -1.0f, 1.0f)));
  flyYaw = glm::degrees(atan2(dir.x, dir.y));

  flyFront = dir;
  flyRight = glm::normalize(glm::cross(flyFront, WORLD_UP));
  flyUp = glm::normalize(glm::cross(flyRight, flyFront));
}

void EditorCamera::endFlyMode() {
  mode = Mode::Orbit;
  // Update orbit params from fly camera state
  orbitTarget = flyPosition + flyFront * orbitDistance;
  orbitYaw = flyYaw + 180.0f;
  orbitPitch = -flyPitch;
  updateOrbitPosition();
}

void EditorCamera::flyMoveForward(float deltaTime) {
  flyPosition += flyFront * flySpeed * deltaTime;
}

void EditorCamera::flyMoveBackward(float deltaTime) {
  flyPosition -= flyFront * flySpeed * deltaTime;
}

void EditorCamera::flyMoveLeft(float deltaTime) {
  flyPosition -= flyRight * flySpeed * deltaTime;
}

void EditorCamera::flyMoveRight(float deltaTime) {
  flyPosition += flyRight * flySpeed * deltaTime;
}

void EditorCamera::flyMouseLook(float deltaX, float deltaY) {
  flyYaw += deltaX * mouseSensitivity;
  flyPitch += deltaY * mouseSensitivity;
  flyPitch = glm::clamp(flyPitch, -89.0f, 89.0f);
}

glm::vec3 EditorCamera::getPosition() const {
  return mode == Mode::Orbit ? orbitPosition : flyPosition;
}

glm::mat4 EditorCamera::getViewMatrix() const {
  if (mode == Mode::Orbit) {
    return glm::lookAt(orbitPosition, orbitTarget, WORLD_UP);
  } else {
    return glm::lookAt(flyPosition, flyPosition + flyFront, WORLD_UP);
  }
}

glm::mat4 EditorCamera::getProjectionMatrix(float aspectRatio) const {
  return glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 1000.0f);
}

Ray EditorCamera::screenToWorldRay(float localX, float localY, float vpWidth, float vpHeight) const {
  // Convert pixel coords to NDC [-1, 1]
  // Screen Y increases downward; OpenGL NDC Y increases upward, so flip Y
  float ndcX = (2.0f * localX) / vpWidth - 1.0f;
  float ndcY = 1.0f - (2.0f * localY) / vpHeight;

  float aspect = vpWidth / vpHeight;
  glm::mat4 proj = getProjectionMatrix(aspect);
  glm::mat4 view = getViewMatrix();

  glm::mat4 invProjView = glm::inverse(proj * view);

  glm::vec4 nearPoint = invProjView * glm::vec4(ndcX, ndcY, 0.0f, 1.0f);
  glm::vec4 farPoint = invProjView * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);

  nearPoint /= nearPoint.w;
  farPoint /= farPoint.w;

  glm::vec3 origin = glm::vec3(nearPoint);
  glm::vec3 direction = glm::normalize(glm::vec3(farPoint) - glm::vec3(nearPoint));

  return { origin, direction };
}

} // namespace sauce::editor
