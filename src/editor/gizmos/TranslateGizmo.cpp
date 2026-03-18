#include <cmath>
#include <editor/gizmos/TranslateGizmo.hpp>

namespace sauce::editor {

    GizmoMeshData TranslateGizmo::generateMesh() const {
        GizmoMeshData data;
        constexpr float PI = 3.14159265f;

        auto addArrow = [&](GizmoAxis axis) {
            glm::vec3 dir = axisDirection(axis);
            glm::vec3 color = axisColor(axis);

            // Build a local coordinate frame for the cylinder cross-section
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

                // Bottom ring vertex
                sauce::Vertex v0{};
                v0.position = offset;
                v0.normal = normal;
                v0.color = color;
                data.vertices.push_back(v0);

                // Top ring vertex
                sauce::Vertex v1{};
                v1.position = dir * SHAFT_LENGTH + offset;
                v1.normal = normal;
                v1.color = color;
                data.vertices.push_back(v1);
            }

            // Shaft indices
            for (int i = 0; i < SEGMENTS; ++i) {
                uint32_t b = base + static_cast<uint32_t>(i) * 2;
                data.indices.push_back(b);
                data.indices.push_back(b + 1);
                data.indices.push_back(b + 2);
                data.indices.push_back(b + 1);
                data.indices.push_back(b + 3);
                data.indices.push_back(b + 2);
            }

            // --- Cone tip ---
            uint32_t coneBase = static_cast<uint32_t>(data.vertices.size());
            glm::vec3 coneStart = dir * SHAFT_LENGTH;
            glm::vec3 coneTip = dir * (SHAFT_LENGTH + CONE_LENGTH);

            // Cone ring vertices
            for (int i = 0; i <= SEGMENTS; ++i) {
                float angle = 2.0f * PI * static_cast<float>(i) / static_cast<float>(SEGMENTS);
                float c = std::cos(angle);
                float s = std::sin(angle);
                glm::vec3 offset = (right * c + up * s) * CONE_RADIUS;
                // Approximate cone normal
                glm::vec3 sideDir = glm::normalize(offset);
                glm::vec3 normal = glm::normalize(sideDir * CONE_LENGTH + dir * CONE_RADIUS);

                sauce::Vertex v{};
                v.position = coneStart + offset;
                v.normal = normal;
                v.color = color;
                data.vertices.push_back(v);
            }

            // Tip vertex
            uint32_t tipIdx = static_cast<uint32_t>(data.vertices.size());
            sauce::Vertex tipV{};
            tipV.position = coneTip;
            tipV.normal = dir;
            tipV.color = color;
            data.vertices.push_back(tipV);

            // Cone side indices
            for (int i = 0; i < SEGMENTS; ++i) {
                data.indices.push_back(coneBase + static_cast<uint32_t>(i));
                data.indices.push_back(tipIdx);
                data.indices.push_back(coneBase + static_cast<uint32_t>(i) + 1);
            }

            // Cone base cap
            uint32_t capCenter = static_cast<uint32_t>(data.vertices.size());
            sauce::Vertex centerV{};
            centerV.position = coneStart;
            centerV.normal = -dir;
            centerV.color = color;
            data.vertices.push_back(centerV);

            for (int i = 0; i < SEGMENTS; ++i) {
                data.indices.push_back(capCenter);
                data.indices.push_back(coneBase + static_cast<uint32_t>(i) + 1);
                data.indices.push_back(coneBase + static_cast<uint32_t>(i));
            }
        };

        addArrow(GizmoAxis::X);
        addArrow(GizmoAxis::Y);
        addArrow(GizmoAxis::Z);

        return data;
    }

    GizmoAxis TranslateGizmo::hitTest(const Ray& ray, const glm::vec3& position,
                                      const glm::quat& /*rotation*/, float scale) const {
        float hitThreshold = 0.08f * scale;
        float shaftLen = (SHAFT_LENGTH + CONE_LENGTH) * scale;

        float bestDist = hitThreshold;
        GizmoAxis bestAxis = GizmoAxis::None;

        for (auto axis : {GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z}) {
            glm::vec3 dir = axisDirection(axis);
            float dist = distanceRayToLine(ray, position, dir, shaftLen);
            if (dist < bestDist) {
                bestDist = dist;
                bestAxis = axis;
            }
        }
        return bestAxis;
    }

    void TranslateGizmo::beginInteraction(GizmoAxis axis, const Ray& ray,
                                          const glm::vec3& entityPos,
                                          const glm::quat& /*rotation*/) {
        activeAxis = axis;
        interacting = true;
        lastEntityPos = entityPos;
        initialT = projectRayOntoAxis(ray, entityPos, axisDirection(axis));
    }

    glm::vec3 TranslateGizmo::updateInteraction(const Ray& ray, const glm::vec3& entityPos,
                                                const glm::quat& /*rotation*/) {
        if (!interacting || activeAxis == GizmoAxis::None)
            return glm::vec3(0.0f);

        glm::vec3 dir = axisDirection(activeAxis);
        float currentT = projectRayOntoAxis(ray, lastEntityPos, dir);
        float deltaT = currentT - initialT;
        initialT = currentT;
        lastEntityPos = entityPos;
        return dir * deltaT;
    }

    void TranslateGizmo::endInteraction() {
        interacting = false;
        activeAxis = GizmoAxis::None;
    }

} // namespace sauce::editor
