#include <physics/constraints/Constraint.hpp>

#include <glm/glm.hpp>

namespace physics {

    struct BendConstraint : public Constraint {
        virtual void solve(std::vector<physics::Vertex>& vertices,
                           std::vector<glm::vec3>& lagrangeMultipliers,
                           float deltatime) const override;

        float restAngle;
    };

} // namespace physics
