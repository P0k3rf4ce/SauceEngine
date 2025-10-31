#include <gtest/gtest.h>
#include <Eigen/Dense>
#include "animation/AnimationProperties.hpp"

using namespace animation;

namespace {

// Helper function to compute a bounding box from a set of points
Eigen::AlignedBox3d computeBoxFromPoints(const std::vector<Eigen::Vector3d>& points) {
    Eigen::AlignedBox3d box;
    box.setEmpty();
    for (const auto& p : points)
        box.extend(p);
    return box;
}

} // namespace

TEST(AnimationPropertiesTest, BoundingBoxRepresentation) {
    // Define 3D points forming a simple rectangular region
    std::vector<Eigen::Vector3d> points = {
        {0.0, 0.0, 0.0},
        {1.0, 2.0, -1.0},
        {0.5, 1.0, 0.0},
        {1.0, 0.0, 1.0}
    };

    Eigen::AlignedBox3d box = computeBoxFromPoints(points);

    Eigen::Vector3d expectedMin(0.0, 0.0, -1.0);
    Eigen::Vector3d expectedMax(1.0, 2.0, 1.0);

    EXPECT_NEAR((box.min() - expectedMin).norm(), 0.0, 1e-9);
    EXPECT_NEAR((box.max() - expectedMax).norm(), 0.0, 1e-9);
}

TEST(AnimationPropertiesTest, BoundingBoxOverlap) {
    AnimationProperties props;

    std::vector<Eigen::Vector3d> box1Points = {
        {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}
    };
    std::vector<Eigen::Vector3d> box2Points = {
        {0.5, 0.5, 0.5}, {1.5, 1.5, 1.5}
    };
    std::vector<Eigen::Vector3d> box3Points = {
        {2.0, 2.0, 2.0}, {3.0, 3.0, 3.0}
    };

    Eigen::AlignedBox3d box1 = computeBoxFromPoints(box1Points);
    Eigen::AlignedBox3d box2 = computeBoxFromPoints(box2Points);
    Eigen::AlignedBox3d box3 = computeBoxFromPoints(box3Points);

    EXPECT_TRUE(props.boxesOverlap(box1, box2));
    EXPECT_FALSE(props.boxesOverlap(box1, box3));
}

TEST(AnimationPropertiesTest, BoundingBox_EmptyInput) {
    std::vector<Eigen::Vector3d> emptyPoints;
    Eigen::AlignedBox3d box = computeBoxFromPoints(emptyPoints);
    EXPECT_TRUE(box.isEmpty());
}

TEST(AnimationPropertiesTest, BoundingBox_SinglePoint) {
    std::vector<Eigen::Vector3d> singlePoint = { {1.0, 2.0, 3.0} };
    Eigen::AlignedBox3d box = computeBoxFromPoints(singlePoint);
    EXPECT_EQ(box.min(), singlePoint[0]);
    EXPECT_EQ(box.max(), singlePoint[0]);
    EXPECT_FALSE(box.isEmpty());
}

TEST(AnimationPropertiesTest, BoundingBox_AllPointsIdentical) {
    std::vector<Eigen::Vector3d> identicalPoints = {
        {2.0, -1.0, 5.0}, {2.0, -1.0, 5.0}, {2.0, -1.0, 5.0}
    };
    Eigen::AlignedBox3d box = computeBoxFromPoints(identicalPoints);
    EXPECT_EQ(box.min(), identicalPoints[0]);
    EXPECT_EQ(box.max(), identicalPoints[0]);
}

TEST(AnimationPropertiesTest, BoundingBox_NegativeCoordinates) {
    std::vector<Eigen::Vector3d> points = {
        {-5.0, -2.0, -3.0}, {-1.0, -8.0, -7.0}, {-4.0, -3.0, -2.0}
    };
    Eigen::AlignedBox3d box = computeBoxFromPoints(points);
    Eigen::Vector3d expectedMin(-5.0, -8.0, -7.0);
    Eigen::Vector3d expectedMax(-1.0, -2.0, -2.0);
    EXPECT_EQ(box.min(), expectedMin);
    EXPECT_EQ(box.max(), expectedMax);
}

TEST(AnimationPropertiesTest, BoundingBox_TouchingButNotOverlapping) {
    AnimationProperties props;

    std::vector<Eigen::Vector3d> box1Points = {
        {0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}
    };
    std::vector<Eigen::Vector3d> box2Points = {
        {1.0, 1.0, 1.0}, {2.0, 2.0, 2.0}
    };

    Eigen::AlignedBox3d box1 = computeBoxFromPoints(box1Points);
    Eigen::AlignedBox3d box2 = computeBoxFromPoints(box2Points);

    // Eigen considers touching boxes as intersecting (they share a boundary)
    EXPECT_TRUE(props.boxesOverlap(box1, box2));
}

TEST(AnimationPropertiesTest, BoundingBox_LargeValues) {
    std::vector<Eigen::Vector3d> points = {
        {1e10, -1e10, 0.0}, {-1e10, 1e10, 1e10}
    };
    Eigen::AlignedBox3d box = computeBoxFromPoints(points);
    EXPECT_EQ(box.min(), Eigen::Vector3d(-1e10, -1e10, 0.0));
    EXPECT_EQ(box.max(), Eigen::Vector3d(1e10, 1e10, 1e10));
}

TEST(AnimationPropertiesTest, BoundingBox_PlanarPoints) {
    std::vector<Eigen::Vector3d> points = {
        {0.0, 0.0, 0.0}, {1.0, 2.0, 0.0}, {2.0, 1.0, 0.0}
    };
    Eigen::AlignedBox3d box = computeBoxFromPoints(points);
    EXPECT_EQ(box.min().z(), 0.0);
    EXPECT_EQ(box.max().z(), 0.0);
}
