#pragma once

#include <physics/constraints/Constraint.hpp>

#include <glm/glm.hpp>

#include <cstdint>
#include <limits>

namespace physics {

    // Position Based Dynamics Collision constraint
    struct CollisionConstraint : public Constraint {
        CollisionConstraint() = default;

        // Construct from two vertex indices plus collision geometry.
        CollisionConstraint(uint32_t a, uint32_t b, glm::vec3 normal, float depth,
                            float comp = 0.0f)
            : Constraint(comp), indexA(a), indexB(b), contactNormal(normal),
              penetrationDepth(depth) {
        }

        // Construct from single vertex colliding with a static surface.
        CollisionConstraint(uint32_t a, glm::vec3 contactPt, glm::vec3 normal, float comp = 0.0f)
            : Constraint(comp), indexA(a), indexB(UINT32_MAX), contactPoint(contactPt),
              contactNormal(normal), isStaticCollision(true) {
        }

        void solve(std::vector<physics::Vertex>& vertices, float deltatime) override {
            if (isStaticCollision) {
                solveStatic(vertices, deltatime);
            } else {
                solveDynamic(vertices, deltatime);
            }
        }

        uint32_t indexA = 0;
        uint32_t indexB = 0;
        glm::vec3 contactPoint = glm::vec3(0.0f);
        glm::vec3 contactNormal = glm::vec3(0.0f, 1.0f, 0.0f);
        float penetrationDepth = 0.0f;
        bool isStaticCollision = false;

      private:
        // Dynamic collision, vertex to vertex
        void solveDynamic(std::vector<physics::Vertex>& vertices, float deltatime) {
            if (indexA >= vertices.size() || indexB >= vertices.size())
                return;

            physics::Vertex& va = vertices[indexA];
            physics::Vertex& vb = vertices[indexB];

            const float w1 = va.invMass;
            const float w2 = vb.invMass;
            if (w1 + w2 <= 1e-8f)
                return;

            const float C = glm::dot(va.position - vb.position, contactNormal) - penetrationDepth;

            if (C >= -1e-8f)
                return;

            const float alphaTilde = compliance / (deltatime * deltatime);

            const float denom = w1 + w2 + alphaTilde;
            const float deltaLambda = (-C - alphaTilde * lambda) / denom;

            const glm::vec3 deltaP_a = (w1 * deltaLambda) * contactNormal;
            const glm::vec3 deltaP_b = (w2 * deltaLambda) * contactNormal;

            va.position += deltaP_a;
            vb.position -= deltaP_b;

            // Orientation corrections
            const glm::vec3 dOmega_a = va.invInertiaTensor * glm::cross(contactNormal, deltaP_a);
            const glm::vec3 dOmega_b = vb.invInertiaTensor * glm::cross(contactNormal, deltaP_b);
            va.orientation =
                glm::normalize(va.orientation + 0.5f * glm::quat(0.0f, dOmega_a) * va.orientation);
            vb.orientation =
                glm::normalize(vb.orientation - 0.5f * glm::quat(0.0f, dOmega_b) * vb.orientation);

            lambda += deltaLambda;
        }

        // Static collision, a single vertex against a fixed surface point
        void solveStatic(std::vector<physics::Vertex>& vertices, float deltatime) {
            if (indexA >= vertices.size())
                return;

            physics::Vertex& va = vertices[indexA];
            const float w = va.invMass;
            if (w <= 1e-8f)
                return;

            const float C = glm::dot(va.position - contactPoint, contactNormal);

            if (C >= -1e-8f)
                return;

            const float alphaTilde = compliance / (deltatime * deltatime);

            const float denom = w + alphaTilde;
            const float deltaLambda = (-C - alphaTilde * lambda) / denom;

            const glm::vec3 deltaP = (w * deltaLambda) * contactNormal;
            va.position += deltaP;

            // Orientation correction
            const glm::vec3 r = contactPoint - va.position;
            const glm::vec3 dOmega = va.invInertiaTensor * glm::cross(r, deltaP);
            va.orientation =
                glm::normalize(va.orientation + 0.5f * glm::quat(0.0f, dOmega) * va.orientation);

            lambda += deltaLambda;
        }
    };

} // namespace physics
