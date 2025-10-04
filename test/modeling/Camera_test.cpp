#include <gtest/gtest.h>
#include <gmock/gmock.h>
/* to enable printing GLM stuff as string */
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include "modeling/Camera.hpp"

using namespace std;

class CameraTest : public ::testing::Test {
protected:
    void SetUp() override { }

    void TearDown() override { }
};

TEST_F(CameraTest, Constructor) {
    auto c = Camera(glm::vec3(0.f,0.f,0.f), glm::vec3(1.f,0.f,0.f));
    EXPECT_EQ(c.getRight(),glm::vec3(0.f,0.f,-1.f)); // right of +X is -Z
    c.LookAt(glm::vec3(-1.f,0.f,0.f));
    EXPECT_EQ(c.getRight(),glm::vec3(0.f,0.f,1.f)); // right of -X is +Z
    c.LookAt(glm::vec3(0.f,0.f,1.f));
    EXPECT_EQ(c.getRight(),glm::vec3(1.f,0.f,0.f)); // right of +Z is +X
    c.LookAt(glm::vec3(0.f,0.f,-1.f));
    EXPECT_EQ(c.getRight(),glm::vec3(-1.f,0.f,0.f)); // right of -Z is -X
}

TEST_F(CameraTest, YawPitch) {
    auto c = Camera(glm::vec3(0.f,0.f,0.f), glm::vec3(1.f,0.f,0.f));
    c.LookAt(-90.f,0.f); // test yaw/pitch LookAt
    EXPECT_NEAR(c.getDirection().x,0.f,0.00000005f);
    EXPECT_NEAR(c.getDirection().y,0.f,0.00000005f);
    EXPECT_NEAR(c.getDirection().z,-1.f,0.00000005f);
    EXPECT_NEAR(c.getRight().x,-1.f,0.00000005f);
    EXPECT_NEAR(c.getRight().y,0.f,0.00000005f);
    EXPECT_NEAR(c.getRight().z,0.f,0.00000005f);
}

TEST_F(CameraTest, Translation) {
    auto c = Camera(glm::vec3(0.f,0.f,0.f), glm::vec3(1.f,0.f,0.f));
    c.translate(1.f,3.f,5.f);
    EXPECT_EQ(c.getPos(),glm::vec3(1.f,3.f,5.f));
    c.translate(glm::vec3(1.f,3.f,5.f));
    EXPECT_EQ(c.getPos(),glm::vec3(2.f,6.f,10.f));
    EXPECT_EQ(c.getRight(),glm::vec3(0.f,0.f,-1.f)); // make sure right direction doesnt get affected when translating
}