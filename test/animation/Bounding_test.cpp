#include <gtest/gtest.h>
#include <Eigen/Dense>

#include "animation/AnimationProperties.hpp"

using namespace animation;

TEST(AnimationPropertiesTest, BoundingBoxRepresentation) {
    AnimationProperties animProps;

    // Define 3D points forming a simple rectangular region
    std::vector<Eigen::Vector3d> points = {
        {0.0, 0.0, 0.0},
        {1.0, 2.0, -1.0},
        {0.5, 1.0, 0.0},
        {1.0, 0.0, 1.0}
    };

    // Compute bounding box
    AABB box = animProps.BoundingBoxRepresentation(points);

    // Expected results
    Eigen::Vector3d expectedMin(0.0, 0.0, -1.0);
    Eigen::Vector3d expectedMax(1.0, 2.0, 1.0);

    EXPECT_NEAR((box.min - expectedMin).norm(), 0.0, 1e-9);
    EXPECT_NEAR((box.max - expectedMax).norm(), 0.0, 1e-9);
}

TEST(AnimationPropertiesTest, BoundingBoxOverlap) {
    // Two sets of points forming boxes that overlap
    std::vector<Eigen::Vector3d> box1Points = {
        {0.0, 0.0, 0.0},
        {1.0, 1.0, 1.0}
    };
    std::vector<Eigen::Vector3d> box2Points = {
        {0.5, 0.5, 0.5},
        {1.5, 1.5, 1.5}
    };
    std::vector<Eigen::Vector3d> box3Points = {
        {2.0, 2.0, 2.0},
        {3.0, 3.0, 3.0}
    };

    // Expect overlap between box1 and box2
    EXPECT_TRUE(AnimationProperties::BoundingBoxOverlap(box1Points, box2Points));

    // Expect no overlap between box1 and box3
    EXPECT_FALSE(AnimationProperties::BoundingBoxOverlap(box1Points, box3Points));
}

// Below are edges cases.

TEST(AnimationPropertiesTest, BoundingBox_EmptyInput) {
    std::vector<Eigen::Vector3d> emptyPoints;
    AABB box = AnimationProperties::BoundingBoxRepresentation(emptyPoints);
    EXPECT_TRUE(box.empty);
}

TEST(AnimationPropertiesTest, BoundingBox_SinglePoint) {
    std::vector<Eigen::Vector3d> singlePoint = { {1.0, 2.0, 3.0} };
    AABB box = AnimationProperties::BoundingBoxRepresentation(singlePoint);
    EXPECT_EQ(box.min, singlePoint[0]);
    EXPECT_EQ(box.max, singlePoint[0]);
    EXPECT_FALSE(box.empty);
}

TEST(AnimationPropertiesTest, BoundingBox_AllPointsIdentical) {
    std::vector<Eigen::Vector3d> identicalPoints = { {2.0, -1.0, 5.0}, {2.0, -1.0, 5.0}, {2.0, -1.0, 5.0} };
    AABB box = AnimationProperties::BoundingBoxRepresentation(identicalPoints);
    EXPECT_EQ(box.min, identicalPoints[0]);
    EXPECT_EQ(box.max, identicalPoints[0]);
}

TEST(AnimationPropertiesTest, BoundingBox_NegativeCoordinates) {
    std::vector<Eigen::Vector3d> points = { {-5.0, -2.0, -3.0}, {-1.0, -8.0, -7.0}, {-4.0, -3.0, -2.0} };
    AABB box = AnimationProperties::BoundingBoxRepresentation(points);
    Eigen::Vector3d expectedMin(-5.0, -8.0, -7.0);
    Eigen::Vector3d expectedMax(-1.0, -2.0, -2.0);
    EXPECT_EQ(box.min, expectedMin);
    EXPECT_EQ(box.max, expectedMax);
}

TEST(AnimationPropertiesTest, BoundingBox_TouchingButNotOverlapping) {
    std::vector<Eigen::Vector3d> box1 = { {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0} };
    std::vector<Eigen::Vector3d> box2 = { {1.0, 1.0, 1.0}, {2.0, 2.0, 2.0} };
    // They touch at the corner but do not overlap
    EXPECT_TRUE(AnimationProperties::BoundingBoxOverlap(box1, box2));
}

TEST(AnimationPropertiesTest, BoundingBox_LargeValues) {
    std::vector<Eigen::Vector3d> points = {
        {1e10, -1e10, 0.0},
        {-1e10, 1e10, 1e10}
    };
    AABB box = AnimationProperties::BoundingBoxRepresentation(points);
    EXPECT_EQ(box.min, Eigen::Vector3d(-1e10, -1e10, 0.0));
    EXPECT_EQ(box.max, Eigen::Vector3d(1e10, 1e10, 1e10));
}

TEST(AnimationPropertiesTest, BoundingBox_PlanarPoints) {
    std::vector<Eigen::Vector3d> points = {
        {0.0, 0.0, 0.0},
        {1.0, 2.0, 0.0},
        {2.0, 1.0, 0.0}
    };
    AABB box = AnimationProperties::BoundingBoxRepresentation(points);
    EXPECT_EQ(box.min.z(), 0.0);
    EXPECT_EQ(box.max.z(), 0.0);
}