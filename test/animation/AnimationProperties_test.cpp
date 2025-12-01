#include <gtest/gtest.h>

#include "animation/AnimationProperties.hpp"
#include "modeling/ModelProperties.hpp"

using namespace animation;

TEST(AnimationPropertiesTest, ComputeCentreOfMassAndVolumeEmptyMesh) {
    std::vector<Eigen::Vector3d> vertices;
    std::vector<unsigned int> indices;
    Eigen::Vector3d com;
    double volume;

    AnimationProperties::computeCenreOfMassAndVolume(vertices, indices, com, volume);

    EXPECT_EQ(com, Eigen::Vector3d(0, 0, 0));
    EXPECT_EQ(volume, 0);
}

TEST(AnimationPropertiesTest, ComputeCenterOfMassAndVolumeOfTetrahedron) {
    std::vector<Eigen::Vector3d> vertices = {
        Eigen::Vector3d(0, 0, 0),
        Eigen::Vector3d(1, 0, 0),
        Eigen::Vector3d(0, 1, 0),
        Eigen::Vector3d(0, 0, 1)
    };
    std::vector<unsigned int> indices = {
        0, 1, 2,
        0, 1, 3,
        0, 2, 3,
        1, 2, 3
    };

    Eigen::Vector3d expectedCom(0.25, 0.25, 0.25), actualCom;
    double expectedVolume = 1.0 / 6.0, actualVolume;

    AnimationProperties::computeCenreOfMassAndVolume(vertices, indices, actualCom, actualVolume);

    EXPECT_NEAR(actualCom.x(), expectedCom.x(), 1e-6);
    EXPECT_NEAR(actualCom.y(), expectedCom.y(), 1e-6);
    EXPECT_NEAR(actualCom.z(), expectedCom.z(), 1e-6);
    EXPECT_NEAR(actualVolume, expectedVolume, 1e-6);
}