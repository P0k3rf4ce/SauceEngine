#include <gtest/gtest.h>
#include "animation/AnimationProperties.hpp"
#include <Eigen/Dense>
#include <vector>

using namespace animation;

TEST(AnimationPropertiesTests, CubeInertiaTensor) {
    /**
     * @brief Test inertia tensor computation for a unit cube centered at origin
     * Cube vertices: (-1,-1,-1) to (1,1,1), mass = 1.0
     * The analytical inertia tensor for a cube of side 2 about COM:
     * I = (1/6)*m*side^2 * Identity(3) = 2/3 * Identity
     */
    
    std::vector<Eigen::Vector3d> vertices = {
        {-1,-1,-1}, {1,-1,-1}, {1,1,-1}, {-1,1,-1},
        {-1,-1,1}, {1,-1,1}, {1,1,1}, {-1,1,1}
    };

    std::vector<Eigen::Vector3i> triangles = {
        {0,1,2}, {0,2,3}, // bottom
        {4,5,6}, {4,6,7}, // top
        {0,1,5}, {0,5,4}, // front
        {2,3,7}, {2,7,6}, // back
        {0,3,7}, {0,7,4}, // left
        {1,2,6}, {1,6,5}  // right
    };

    Eigen::Vector3d com(0,0,0);
    AnimationProperties animPropsMock; // default constructor if available
    
    Eigen::Matrix3d I = animPropsMock.computeInertiaTensor(vertices, triangles, com);
    
    // For a unit cube of side=2 and mass=1: Ixx = Iyy = Izz = 2/3
    double expected = 2.0 / 3.0;

    EXPECT_NEAR(I(0,0), expected, 1e-6);
    EXPECT_NEAR(I(1,1), expected, 1e-6);
    EXPECT_NEAR(I(2,2), expected, 1e-6);

    // Off-diagonal terms should be zero
    EXPECT_NEAR(I(0,1), 0.0, 1e-6);
    EXPECT_NEAR(I(0,2), 0.0, 1e-6);
    EXPECT_NEAR(I(1,0), 0.0, 1e-6);
    EXPECT_NEAR(I(1,2), 0.0, 1e-6);
    EXPECT_NEAR(I(2,0), 0.0, 1e-6);
    EXPECT_NEAR(I(2,1), 0.0, 1e-6);
}

TEST(AnimationPropertiesTests, InverseTensor) {
    /**
     * @brief Test that the inverse inertia tensor is correct
     */
    Eigen::Matrix3d I;
    I << 2.0, 0, 0,
         0, 3.0, 0,
         0, 0, 4.0;

    AnimationProperties animPropsMock; // default constructor
    animPropsMock.setInertiaTensor(I); // assume you add a setter for testing

    Eigen::Matrix3d Iinv = animPropsMock.getInverseInertiaTensor();

    Eigen::Matrix3d identity = I * Iinv;
    EXPECT_NEAR(identity(0,0), 1.0, 1e-6);
    EXPECT_NEAR(identity(1,1), 1.0, 1e-6);
    EXPECT_NEAR(identity(2,2), 1.0, 1e-6);
    EXPECT_NEAR(identity(0,1), 0.0, 1e-6);
    EXPECT_NEAR(identity(0,2), 0.0, 1e-6);
    EXPECT_NEAR(identity(1,0), 0.0, 1e-6);
    EXPECT_NEAR(identity(1,2), 0.0, 1e-6);
    EXPECT_NEAR(identity(2,0), 0.0, 1e-6);
    EXPECT_NEAR(identity(2,1), 0.0, 1e-6);
}

TEST(AnimationPropertiesTests, NonZeroCOM) {
    /**
     * @brief Test that tensor computation works correctly with a non-zero center of mass
     */
    std::vector<Eigen::Vector3d> vertices = {{1,1,1}, {2,1,1}, {1,2,1}};
    std::vector<Eigen::Vector3i> triangles = {{0,1,2}};
    Eigen::Vector3d com(1,1,1);

    AnimationProperties animPropsMock;
    Eigen::Matrix3d I = animPropsMock.computeInertiaTensor(vertices, triangles, com);

    // Diagonal elements should be small but non-zero due to shifted COM
    EXPECT_GT(I(0,0), 0.0);
    EXPECT_GT(I(1,1), 0.0);
    EXPECT_GT(I(2,2), 0.0);
}
