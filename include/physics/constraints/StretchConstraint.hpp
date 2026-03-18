#pragma once

#include <physics/constraints/Constraint.hpp>

#include <glm/glm.hpp>

#include <cstdint>
#include <limits>

namespace physics {

    struct StretchConstraint : public Constraint {
        StretchConstraint() = default;

        StretchConstraint(uint32_t a, uint32_t b, float rest, float comp = 0.0f)
            : Constraint(comp), indexA(a), indexB(b), restLength(rest) {
        }

        void solve(std::vector<physics::Vertex>& vertices, float deltatime) override {
            if (indexA >= vertices.size() || indexB >= vertices.size())
                return;

            physics::Vertex& va = vertices[indexA];
            physics::Vertex& vb = vertices[indexB];

            const float w1 = va.invMass;
            const float w2 = vb.invMass;
            if (w1 + w2 <= 0.0f)
                return; // both static

            const glm::vec3 diff = va.position - vb.position;
            const float dist = glm::length(diff);
            if (dist <= std::numeric_limits<float>::epsilon())
                return;

            // Constraint value
            const float C = dist - restLength;

            // Regularisation  α̃ = α / h²
            const float alphaTilde = compliance / (deltatime * deltatime);

            // Newton–Raphson Δλ
            const float denom = w1 + w2 + alphaTilde;
            const float deltaLambda = (-C - alphaTilde * lambda) / denom;

            // Gradient direction (unit)
            const glm::vec3 n = diff / dist;

            // Apply position corrections  Δx = w Δλ ∇C
            va.position += (w1 * deltaLambda) * n;
            vb.position -= (w2 * deltaLambda) * n;

            lambda += deltaLambda;
        }

        uint32_t indexA = 0;
        uint32_t indexB = 0;
        float restLength = 0.0f;
    };

} // namespace physics
