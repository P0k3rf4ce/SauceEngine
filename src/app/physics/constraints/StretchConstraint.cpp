#include "app/physics/constraints/StretchConstraint.hpp"

namespace{
    constexpr float EPS = 1e-8f;
}

/**
 * Preconditions:
 * 1. vertices.length() == 2 (this constraint uses exactly two vertices) 
 * 2. lagrangeMultiplers.length() == 1 ()
 */
void StretchConstraint::solve(std::vector<Vertex>& vertices, std::vector<glm::vec3>& lagrangeMultipliers, float deltatime) const {
    Vertex& v1 = vertices[0];
    Vertex& v2 = vertices[1];

    float w1 = v1.invMass;
    float w2 = v2.invMass;

    // No point in calculating anything then
    if (w1 < EPS && w2 < EPS) return;

    glm::vec3 p1 = v1.position;
    glm::vec3 p2 = v2.position;

    glm::vec3 diff = p1 - p2;
    float distance = glm::length(diff);

    // distance is negligible in this case
    if (distance < EPS) return; 

    // calculate the constraint
    float c = distance - restLength;

    glm::vec3 g1 = diff/distance;
    glm::vec3 g2 = -g1;

    float a = compliance/(deltatime*deltatime);

    float& lambda = lagrangeMultipliers[0].x;

    float denominator = w1 + w2 + a;
    // negligible in this case
    if (denominator < EPS) return; 

    float dlambda = (-c - a * lambda)/denominator;

    lambda += dlambda;

    v1.position += w1 * dlambda * g1;
    v2.position += w2 * dlambda * g2;

}