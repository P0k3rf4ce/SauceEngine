#include <gtest/gtest.h>

#include "animation/AnimationProperties.hpp"

using namespace animation;

TEST(AnimationPropertiesTest, TetrahedronInertia) {
    AnimationProperties animProps;

    // Define vertices of a unit tetrahedron
    std::vector<Eigen::Vector3d> vertices = {
        {0.0, 0.0, 0.0},  // v0
        {1.0, 0.0, 0.0},  // v1
        {0.0, 1.0, 0.0},  // v2
        {0.0, 0.0, 1.0}   // v3
    };

    // Triangle indices with consistent orinetation
    std::vector<unsigned int> indices = {
        0,2,1,
        0,1,3,
        0,3,2,
        1,2,3
    };

    // Center of mass
    Eigen::Vector3d com(0.25, 0.25, 0.25);

    // Compute inertia tensor
    Eigen::Matrix3d inertia = animProps.computeInertiaTensor(vertices, indices, com);

    // Expected inertia tensor (matches your function for density = 1)
    Eigen::Matrix3d expected;
    expected << 0.0125,       0.00208333,  0.00208333,
                0.00208333,   0.0125,      0.00208333,
                0.00208333,   0.00208333,  0.0125;

    // A = 0 iff norm(A) = 0
    EXPECT_NEAR((inertia - expected).norm(), 0.0, 1e-6f);
}

TEST(AnimationPropertiesTest, InverseInertiaTensor) {
    AnimationProperties animProps;

    // Better-conditioned symmetric positive definite matrix
    Eigen::Matrix3d inertia;
    inertia << 1.0, 0.01, 0.01,
               0.01, 1.0, 0.01,
               0.01, 0.01, 1.0;

    // Compute inverse
    Eigen::Matrix3d inverse = animProps.computeInverseInertiaTensor(inertia);

    // Identity check
    Eigen::Matrix3d identityCheck = inertia * inverse;
    Eigen::Matrix3d expectedIdentity = Eigen::Matrix3d::Identity();

    EXPECT_NEAR((identityCheck - expectedIdentity).norm(), 0.0, 1e-6);
    EXPECT_NEAR((inverse - inertia.inverse()).norm(), 0.0, 1e-6);
}
