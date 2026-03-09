#pragma once

#include <glm/glm.hpp>
#include <app/Vertex.hpp>
#include <editor/EditorCamera.hpp>
#include <vector>
#include <cmath>
#include <glm/gtc/quaternion.hpp>

namespace sauce::editor {

enum class GizmoType { Translate, Rotate, Scale };
enum class GizmoAxis { None, X, Y, Z };

struct GizmoMeshData {
  std::vector<sauce::Vertex> vertices;
  std::vector<uint32_t> indices;
};

class Gizmo {
public:
  virtual ~Gizmo() = default;
  virtual GizmoType getType() const = 0;

  virtual GizmoMeshData generateMesh() const = 0;

  virtual GizmoAxis hitTest(const Ray& ray,
                            const glm::vec3& position, const glm::quat& rotation, float scale) const = 0;

  virtual void beginInteraction(GizmoAxis axis, const Ray& ray, const glm::vec3& entityPos, const glm::quat& rotation) = 0;
  virtual glm::vec3 updateInteraction(const Ray& ray, const glm::vec3& entityPos, const glm::quat& rotation) = 0;
  virtual void endInteraction() = 0;

  bool isInteracting() const { return interacting; }
  GizmoAxis getActiveAxis() const { return activeAxis; }

protected:
  bool interacting = false;
  GizmoAxis activeAxis = GizmoAxis::None;

  static glm::vec3 axisDirection(GizmoAxis axis) {
    switch (axis) {
      case GizmoAxis::X: return { 1, 0, 0 };
      case GizmoAxis::Y: return { 0, 1, 0 };
      case GizmoAxis::Z: return { 0, 0, 1 };
      default: return { 0, 0, 0 };
    }
  }

  static glm::vec3 axisColor(GizmoAxis axis) {
    switch (axis) {
      case GizmoAxis::X: return { 1, 0, 0 };
      case GizmoAxis::Y: return { 0, 1, 0 };
      case GizmoAxis::Z: return { 0, 0, 1 };
      default: return { 1, 1, 1 };
    }
  }

  // Project a ray onto an axis line through origin, return parameter t along the axis
  static float projectRayOntoAxis(const Ray& ray, const glm::vec3& axisOrigin,
                                   const glm::vec3& axisDir) {
    // Closest point between two lines:
    // Line 1: axisOrigin + t * axisDir
    // Line 2: ray.origin + s * ray.direction
    glm::vec3 w = axisOrigin - ray.origin;
    float a = glm::dot(axisDir, axisDir);
    float b = glm::dot(axisDir, ray.direction);
    float c = glm::dot(ray.direction, ray.direction);
    float d = glm::dot(axisDir, w);
    float e = glm::dot(ray.direction, w);
    float denom = a * c - b * b;
    if (std::abs(denom) < 1e-8f) return 0.0f;
    return (b * e - c * d) / denom;
  }

  // Distance from ray to axis line (for hit testing)
  static float distanceRayToLine(const Ray& ray, const glm::vec3& lineOrigin,
                                  const glm::vec3& lineDir, float maxT = 1.0f) {
    // Clamp parameter t on axis line to [0, maxT] (shaft length)
    float t = projectRayOntoAxis(ray, lineOrigin, lineDir);
    t = glm::clamp(t, 0.0f, maxT);

    glm::vec3 pointOnAxis = lineOrigin + t * lineDir;

    // Find closest point on ray to pointOnAxis
    float s = glm::dot(pointOnAxis - ray.origin, ray.direction);
    if (s < 0.0f) s = 0.0f;
    glm::vec3 pointOnRay = ray.origin + s * ray.direction;

    return glm::length(pointOnAxis - pointOnRay);
  }
};

} // namespace sauce::editor
