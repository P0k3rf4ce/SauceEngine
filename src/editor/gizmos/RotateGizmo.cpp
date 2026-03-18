#include <cmath>
#include <editor/gizmos/RotateGizmo.hpp>

namespace sauce::editor {

    GizmoMeshData RotateGizmo::generateMesh() const {
        GizmoMeshData data;
        constexpr float PI = 3.14159265f;

        auto addRing = [&](GizmoAxis axis) {
            glm::vec3 normal = axisDirection(axis);
            glm::vec3 color = axisColor(axis);

            // Build tangent/bitangent for the ring plane
            glm::vec3 tangent =
                (std::abs(normal.z) < 0.9f) ? glm::vec3(0, 0, 1) : glm::vec3(1, 0, 0);
            tangent = glm::normalize(tangent - normal * glm::dot(tangent, normal));
            glm::vec3 bitangent = glm::cross(normal, tangent);

            uint32_t base = static_cast<uint32_t>(data.vertices.size());

            for (int i = 0; i <= RING_SEGMENTS; ++i) {
                float ringAngle =
                    2.0f * PI * static_cast<float>(i) / static_cast<float>(RING_SEGMENTS);
                // Center of tube cross-section on the ring
                glm::vec3 ringCenter =
                    (tangent * std::cos(ringAngle) + bitangent * std::sin(ringAngle)) * RING_RADIUS;
                glm::vec3 ringNormal = glm::normalize(ringCenter);

                // Build tube cross-section
                glm::vec3 tubeUp = normal;
                glm::vec3 tubeRight = ringNormal;

                for (int j = 0; j <= TUBE_SEGMENTS; ++j) {
                    float tubeAngle =
                        2.0f * PI * static_cast<float>(j) / static_cast<float>(TUBE_SEGMENTS);
                    glm::vec3 offset =
                        (tubeRight * std::cos(tubeAngle) + tubeUp * std::sin(tubeAngle)) *
                        TUBE_RADIUS;

                    sauce::Vertex v{};
                    v.position = ringCenter + offset;
                    v.normal = glm::normalize(offset);
                    v.color = color;
                    data.vertices.push_back(v);
                }
            }

            // Indices: connect adjacent ring segments
            uint32_t stride = TUBE_SEGMENTS + 1;
            for (int i = 0; i < RING_SEGMENTS; ++i) {
                for (int j = 0; j < TUBE_SEGMENTS; ++j) {
                    uint32_t a =
                        base + static_cast<uint32_t>(i) * stride + static_cast<uint32_t>(j);
                    uint32_t b = a + stride;
                    uint32_t c = a + 1;
                    uint32_t d = b + 1;
                    data.indices.push_back(a);
                    data.indices.push_back(b);
                    data.indices.push_back(c);
                    data.indices.push_back(c);
                    data.indices.push_back(b);
                    data.indices.push_back(d);
                }
            }
        };

        addRing(GizmoAxis::X);
        addRing(GizmoAxis::Y);
        addRing(GizmoAxis::Z);

        return data;
    }

    GizmoAxis RotateGizmo::hitTest(const Ray& ray, const glm::vec3& position,
                                   const glm::quat& /*rotation*/, float scale) const {
        float hitThreshold = 0.1f * scale;
        float bestDist = hitThreshold;
        GizmoAxis bestAxis = GizmoAxis::None;

        for (auto axis : {GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z}) {
            glm::vec3 normal = axisDirection(axis);
            float radius = RING_RADIUS * scale;

            // Intersect ray with the ring's plane
            float denom = glm::dot(normal, ray.direction);
            if (std::abs(denom) < 1e-6f)
                continue;

            float t = glm::dot(position - ray.origin, normal) / denom;
            if (t < 0.0f)
                continue;

            glm::vec3 hitPoint = ray.origin + t * ray.direction;
            float distFromCenter = glm::length(hitPoint - position);
            float ringDist = std::abs(distFromCenter - radius);

            if (ringDist < bestDist) {
                bestDist = ringDist;
                bestAxis = axis;
            }
        }
        return bestAxis;
    }

    float RotateGizmo::angleOnPlane(const Ray& ray, const glm::vec3& center,
                                    const glm::vec3& normal) const {
        float denom = glm::dot(normal, ray.direction);
        if (std::abs(denom) < 1e-6f)
            return 0.0f;

        float t = glm::dot(center - ray.origin, normal) / denom;
        glm::vec3 hitPoint = ray.origin + t * ray.direction;
        glm::vec3 dir = hitPoint - center;

        // Build local 2D axes on the plane
        glm::vec3 tangent = (std::abs(normal.z) < 0.9f) ? glm::vec3(0, 0, 1) : glm::vec3(1, 0, 0);
        tangent = glm::normalize(tangent - normal * glm::dot(tangent, normal));
        glm::vec3 bitangent = glm::cross(normal, tangent);

        return std::atan2(glm::dot(dir, bitangent), glm::dot(dir, tangent));
    }

    void RotateGizmo::beginInteraction(GizmoAxis axis, const Ray& ray, const glm::vec3& entityPos,
                                       const glm::quat& /*rotation*/) {
        activeAxis = axis;
        interacting = true;
        lastEntityPos = entityPos;
        lastAngle = angleOnPlane(ray, entityPos, axisDirection(axis));
    }

    glm::vec3 RotateGizmo::updateInteraction(const Ray& ray, const glm::vec3& entityPos,
                                             const glm::quat& /*rotation*/) {
        if (!interacting || activeAxis == GizmoAxis::None)
            return glm::vec3(0.0f);

        glm::vec3 normal = axisDirection(activeAxis);
        float currentAngle = angleOnPlane(ray, lastEntityPos, normal);
        float delta = currentAngle - lastAngle;

        // Wrap around
        if (delta > 3.14159f)
            delta -= 2.0f * 3.14159f;
        if (delta < -3.14159f)
            delta += 2.0f * 3.14159f;

        lastAngle = currentAngle;
        lastEntityPos = entityPos;

        // Return angle delta packed into the axis component
        return normal * delta;
    }

    void RotateGizmo::endInteraction() {
        interacting = false;
        activeAxis = GizmoAxis::None;
    }

} // namespace sauce::editor
