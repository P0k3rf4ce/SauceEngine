#pragma once

#include <algorithm>
#include <app/Vertex.hpp>
#include <cmath>
#include <glm/glm.hpp>
#include <limits>
#include <vector>

namespace sauce::editor {

    struct AABB {
        glm::vec3 min;
        glm::vec3 max;

        static AABB fromVertices(const std::vector<sauce::Vertex>& vertices) {
            glm::vec3 lo(std::numeric_limits<float>::max());
            glm::vec3 hi(std::numeric_limits<float>::lowest());
            for (const auto& v : vertices) {
                lo = glm::min(lo, v.position);
                hi = glm::max(hi, v.position);
            }
            return {lo, hi};
        }

        AABB transformed(const glm::mat4& m) const {
            // Transform all 8 corners and recompute bounds
            glm::vec3 corners[8] = {
                {min.x, min.y, min.z}, {max.x, min.y, min.z}, {min.x, max.y, min.z},
                {max.x, max.y, min.z}, {min.x, min.y, max.z}, {max.x, min.y, max.z},
                {min.x, max.y, max.z}, {max.x, max.y, max.z},
            };
            glm::vec3 newMin(std::numeric_limits<float>::max());
            glm::vec3 newMax(std::numeric_limits<float>::lowest());
            for (const auto& c : corners) {
                glm::vec3 t = glm::vec3(m * glm::vec4(c, 1.0f));
                newMin = glm::min(newMin, t);
                newMax = glm::max(newMax, t);
            }
            return {newMin, newMax};
        }
    };

    // Slab method ray-AABB intersection. Returns true if hit; tOut = distance along ray.
    inline bool rayIntersectsAABB(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                                  const AABB& box, float& tOut) {
        float tmin = 0.0f;
        float tmax = std::numeric_limits<float>::max();

        for (int i = 0; i < 3; ++i) {
            if (std::abs(rayDir[i]) < 1e-8f) {
                // Ray is parallel to slab
                if (rayOrigin[i] < box.min[i] || rayOrigin[i] > box.max[i])
                    return false;
            } else {
                float invD = 1.0f / rayDir[i];
                float t1 = (box.min[i] - rayOrigin[i]) * invD;
                float t2 = (box.max[i] - rayOrigin[i]) * invD;
                if (t1 > t2)
                    std::swap(t1, t2);
                tmin = std::max(tmin, t1);
                tmax = std::min(tmax, t2);
                if (tmin > tmax)
                    return false;
            }
        }
        tOut = tmin;
        return true;
    }

} // namespace sauce::editor
