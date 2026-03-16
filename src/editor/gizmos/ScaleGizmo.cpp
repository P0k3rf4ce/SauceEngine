#include <editor/gizmos/ScaleGizmo.hpp>
#include <cmath>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace sauce::editor {

GizmoMeshData ScaleGizmo::generateMesh() const {
  GizmoMeshData data;
  constexpr float PI = 3.14159265f;

  auto addHandle = [&](GizmoAxis axis) {
    glm::vec3 dir = axisDirection(axis);
    glm::vec3 color = axisColor(axis);

    glm::vec3 up = (std::abs(dir.z) < 0.9f) ? glm::vec3(0, 0, 1) : glm::vec3(1, 0, 0);
    glm::vec3 right = glm::normalize(glm::cross(dir, up));
    up = glm::normalize(glm::cross(right, dir));

    uint32_t base = static_cast<uint32_t>(data.vertices.size());

    // --- Shaft (cylinder) ---
    for (int i = 0; i <= SEGMENTS; ++i) {
      float angle = 2.0f * PI * static_cast<float>(i) / static_cast<float>(SEGMENTS);
      float c = std::cos(angle);
      float s = std::sin(angle);
      glm::vec3 offset = (right * c + up * s) * SHAFT_RADIUS;
      glm::vec3 normal = glm::normalize(right * c + up * s);

      sauce::Vertex v0{};
      v0.position = offset;
      v0.normal = normal;
      v0.color = color;
      data.vertices.push_back(v0);

      sauce::Vertex v1{};
      v1.position = dir * SHAFT_LENGTH + offset;
      v1.normal = normal;
      v1.color = color;
      data.vertices.push_back(v1);
    }

    for (int i = 0; i < SEGMENTS; ++i) {
      uint32_t b = base + static_cast<uint32_t>(i) * 2;
      data.indices.push_back(b);
      data.indices.push_back(b + 1);
      data.indices.push_back(b + 2);
      data.indices.push_back(b + 1);
      data.indices.push_back(b + 3);
      data.indices.push_back(b + 2);
    }

    // --- Cube endpoint ---
    glm::vec3 cubeCenter = dir * SHAFT_LENGTH;
    float hs = CUBE_SIZE; // half-size

    // 8 corners of the cube
    glm::vec3 corners[8];
    int idx = 0;
    for (int x = -1; x <= 1; x += 2) {
      for (int y = -1; y <= 1; y += 2) {
        for (int z = -1; z <= 1; z += 2) {
          corners[idx++] = cubeCenter + glm::vec3(x, y, z) * hs;
        }
      }
    }

    // 6 faces (each as 2 triangles)
    // Corner indices: 0(-,-,-) 1(-,-,+) 2(-,+,-) 3(-,+,+) 4(+,-,-) 5(+,-,+) 6(+,+,-) 7(+,+,+)
    static const int faces[6][4] = {
      {0, 1, 3, 2}, // -X
      {4, 6, 7, 5}, // +X
      {0, 4, 5, 1}, // -Y
      {2, 3, 7, 6}, // +Y
      {0, 2, 6, 4}, // -Z
      {1, 5, 7, 3}, // +Z
    };
    static const glm::vec3 faceNormals[6] = {
      {-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}
    };

    for (int f = 0; f < 6; ++f) {
      uint32_t faceBase = static_cast<uint32_t>(data.vertices.size());
      for (int v = 0; v < 4; ++v) {
        sauce::Vertex vert{};
        vert.position = corners[faces[f][v]];
        vert.normal = faceNormals[f];
        vert.color = color;
        data.vertices.push_back(vert);
      }
      data.indices.push_back(faceBase);
      data.indices.push_back(faceBase + 1);
      data.indices.push_back(faceBase + 2);
      data.indices.push_back(faceBase);
      data.indices.push_back(faceBase + 2);
      data.indices.push_back(faceBase + 3);
    }
  };

  addHandle(GizmoAxis::X);
  addHandle(GizmoAxis::Y);
  addHandle(GizmoAxis::Z);

  return data;
}

GizmoAxis ScaleGizmo::hitTest(const Ray& ray, const glm::vec3& position, const glm::quat& rotation,  float scale) const {
  float hitThreshold = 0.08f * scale;
  float shaftLen = SHAFT_LENGTH * scale;

  float bestDist = hitThreshold;
  GizmoAxis bestAxis = GizmoAxis::None;

  for (auto axis : { GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z }) {
    glm::vec3 dir = rotation * axisDirection(axis);
    float dist = distanceRayToLine(ray, position, dir, shaftLen);
    if (dist < bestDist) {
      bestDist = dist;
      bestAxis = axis;
    }
  }
  return bestAxis;
}

void ScaleGizmo::beginInteraction(GizmoAxis axis, const Ray& ray, const glm::vec3& entityPos, const glm::quat& rotation) {
  activeAxis = axis;
  interacting = true;
  lastEntityPos = entityPos;
  glm::vec3 dir = rotation * axisDirection(axis);
  initialT = projectRayOntoAxis(ray, entityPos, dir);
}

glm::vec3 ScaleGizmo::updateInteraction(const Ray& ray, const glm::vec3& entityPos, const glm::quat& rotation) {
  if (!interacting || activeAxis == GizmoAxis::None) return glm::vec3(0.0f);

  glm::vec3 dir = rotation * axisDirection(activeAxis);
  float currentT = projectRayOntoAxis(ray, lastEntityPos, dir);
  float deltaT = currentT - initialT;
  initialT = currentT;
  lastEntityPos = entityPos;

  // Return scale delta along the active axis
  return dir * deltaT;
}

void ScaleGizmo::endInteraction() {
  interacting = false;
  activeAxis = GizmoAxis::None;
}

} // namespace sauce::editor
