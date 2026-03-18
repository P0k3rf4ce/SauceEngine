#include "app/physics/constraints/BendConstraint.hpp"

namespace {
    constexpr float EPS = 1e-8f;
}

/**
 * Preconditions:
 * 1. vertices.length() == 4 (this constraint uses exactly four vertices) 
 * 2. lagrangeMultiplers.length() == 1 ()
 */
void BendConstraint::solve(std::vector<Vertex>& vertices,
                           std::vector<glm::vec3>& lagrangeMultipliers, float deltatime) const {
    Vertex& v1 = vertices[0];
    Vertex& v2 = vertices[1];
    Vertex& v3 = vertices[2];
    Vertex& v4 = vertices[3];

    float w1 = v1.invMass;
    float w2 = v2.invMass;
    float w3 = v3.invMass;
    float w4 = v4.invMass;

    // No point in calculating anything then
    if (w1 + w2 + w3 + w4 < EPS)
        return;

    glm::vec3 p1 = v1.position;
    glm::vec3 p2 = v2.position;
    glm::vec3 p3 = v3.position;
    glm::vec3 p4 = v4.position;

    // helper vectors representing triangle edges
    glm::vec3 e = p2 - p1; // common edge the two triangles share
    float eLen = glm::length(e);
    if (eLen < EPS)
        return;
    glm::vec3 e31 = p3 - p1;
    glm::vec3 e41 = p4 - p1;

    glm::vec3 n1u = glm::cross(e, e31);
    glm::vec3 n2u = glm::cross(e, e41);

    glm::vec3 n1 = n1u; // left term in the acos before dotp
    glm::vec3 n2 = n2u; // right term in the acos before dotp

    // normalize
    float n1Len = glm::length(n1);
    float n2Len = glm::length(n2);
    if (n1Len < EPS || n2Len < EPS)
        return; // values would be negligible
    n1 = n1 / n1Len;
    n2 = n2 / n2Len;

    float dot = glm::clamp(glm::dot(n1, n2), -1.0f, 1.0f);

    float c = std::acos(dot) - restAngle;

    if (std::abs(c) < 1e-6f)
        return; // basically zero
    // more helper values
    float ee = glm::dot(e, e);
    float s3 = glm::dot(e31, e) / ee;
    float s4 = glm::dot(e41, e) / ee;

    //compute gradients
    glm::vec3 g3 = (eLen / (n1Len * n1Len)) * glm::cross(e, n1);
    glm::vec3 g4 = (eLen / (n2Len * n2Len)) * glm::cross(n2, e);
    glm::vec3 g1 = -g3 - g4 - s3 * g3 - s4 * g4;
    glm::vec3 g2 = -g1 - g3 - g4;

    float a = compliance / (deltatime * deltatime);

    float& lambda = lagrangeMultipliers[0].x;

    float denominator = w1 * glm::dot(g1, g1) + w2 * glm::dot(g2, g2) + w3 * glm::dot(g3, g3) +
                        w4 * glm::dot(g4, g4) + a;
    // negligible in this case
    if (denominator < EPS)
        return;

    float dlambda = (-c - a * lambda) / denominator;

    lambda += dlambda;

    v1.position += w1 * dlambda * g1;
    v2.position += w2 * dlambda * g2;
    v3.position += w3 * dlambda * g3;
    v4.position += w4 * dlambda * g4;
}
