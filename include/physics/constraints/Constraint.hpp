#pragma once

#include <physics/Vertex.hpp>

#include <vector>

namespace physics {

    struct Constraint {
        Constraint() = default;
        explicit Constraint(float comp) : compliance(comp) {
        }
        virtual ~Constraint() = default;
        virtual void solve(std::vector<physics::Vertex>& vertices, float deltatime) = 0;
        void resetLambda() {
            lambda = 0.0f;
        }
        float compliance = 0.0f;

      protected:
        float lambda = 0.0f;
    };

} // namespace physics
