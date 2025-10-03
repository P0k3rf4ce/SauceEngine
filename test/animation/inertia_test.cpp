#include <gtest/gtest.h>

#include "animation/AnimationProperties.hpp"

using namespace animation;

// Helper for approximate comparison of Eigen matrices
bool isApprox(const Eigen::Matrix3f &a, const Eigen::Matrix3f &b, float tol = 1e-6f) {
    return (a - b).cwiseAbs().maxCoeff() < tol;
}


TEST(AnimationPropertiesTest, TetrahedronInertia) {
    AnimationProperties animProps;

    // Define vertices of a unit tetrahedron
    std::vector<Eigen::Vector3d> vertices = {
        {0.0, 0.0, 0.0},  // v0
        {1.0, 0.0, 0.0},  // v1
        {0.0, 1.0, 0.0},  // v2
        {0.0, 0.0, 1.0}   // v3
    };

    // Triangle indices
    std::vector<unsigned int> indices = {
        0,1,2,  // base
        0,1,3,
        0,2,3,
        1,2,3
    };

    // Center of mass
    Eigen::Vector3d com(0.25, 0.25, 0.25);

    // Compute inertia tensor
    Eigen::Matrix3f inertia = animProps.computeInertiaTensor(vertices, indices, com);
    std::cout << "Computed inertia:\n" << inertia << std::endl;

    // Expected inertia tensor (matches your function for density = 1)
    Eigen::Matrix3f expected;
    expected << 0.0125f,       -0.00208333f,  0.00625f,
               -0.00208333f,   0.0125f,      -0.00208333f,
                0.00625f,     -0.00208333f,  0.0125f;

    // Compare with tolerance
    EXPECT_TRUE(inertia.isApprox(expected, 1e-6f));
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

    EXPECT_TRUE(identityCheck.isApprox(expectedIdentity, 1e-6));
    EXPECT_TRUE(inverse.isApprox(inertia.inverse(), 1e-6));
}
