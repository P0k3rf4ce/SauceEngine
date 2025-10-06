#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "modeling/Camera.hpp"

using namespace std;
using namespace Eigen;

class CameraTest : public ::testing::Test {
protected:
    void SetUp() override { }

    void TearDown() override { }
};

TEST_F(CameraTest, Constructor) {
    auto c = Camera(Vector3f(0.f,0.f,0.f), Vector3f(1.f,0.f,0.f));
    EXPECT_EQ(c.getRight(),Vector3f(0.f,0.f,-1.f)); // right of +X is -Z
    c.LookAt(Vector3f(-1.f,0.f,0.f));
    EXPECT_EQ(c.getRight(),Vector3f(0.f,0.f,1.f)); // right of -X is +Z
    c.LookAt(Vector3f(0.f,0.f,1.f));
    EXPECT_EQ(c.getRight(),Vector3f(1.f,0.f,0.f)); // right of +Z is +X
    c.LookAt(Vector3f(0.f,0.f,-1.f));
    EXPECT_EQ(c.getRight(),Vector3f(-1.f,0.f,0.f)); // right of -Z is -X
}

TEST_F(CameraTest, YawPitch) {
    auto c = Camera(Vector3f(0.f,0.f,0.f), Vector3f(1.f,0.f,0.f));
    c.LookAt(-90.f,0.f); // test yaw/pitch LookAt
    EXPECT_NEAR(c.getDirection()(0),0.f,0.00000005f);
    EXPECT_NEAR(c.getDirection()(1),0.f,0.00000005f);
    EXPECT_NEAR(c.getDirection()(2),-1.f,0.00000005f);
    EXPECT_NEAR(c.getRight()(0),-1.f,0.00000005f);
    EXPECT_NEAR(c.getRight()(1),0.f,0.00000005f);
    EXPECT_NEAR(c.getRight()(2),0.f,0.00000005f);
}

TEST_F(CameraTest, Translation) {
    auto c = Camera(Vector3f(0.f,0.f,0.f), Vector3f(1.f,0.f,0.f));
    c.translate(1.f,3.f,5.f);
    EXPECT_EQ(c.getPos(),Vector3f(1.f,3.f,5.f));
    c.translate(Vector3f(1.f,3.f,5.f));
    EXPECT_EQ(c.getPos(),Vector3f(2.f,6.f,10.f));
    EXPECT_EQ(c.getRight(),Vector3f(0.f,0.f,-1.f)); // make sure right direction doesnt get affected when translating
}

TEST_F(CameraTest, Rotation) {
    auto c = Camera(Vector3f(0.f,0.f,0.f), Vector3f(1.f,0.f,0.f));
    c.rotateHori(1.5707963267948966); // radians(90 degrees). dont feel like making a public radians function just for testing :)
    EXPECT_NEAR(c.getDirection()(0),0.f,0.00000005f);
    EXPECT_NEAR(c.getDirection()(1),0.f,0.00000005f);
    EXPECT_NEAR(c.getDirection()(2),-1.f,0.00000005f);
}
