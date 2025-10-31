#include <gtest/gtest.h>
#include <vector>
#include <glm/glm.hpp>
#include "animation/DerivFunc.hpp"
#include "animation/CollisionDetection.hpp"

using namespace animation;

static RigidBody makeBody(
    const triple& x = triple(0.f),
    const quaternion& q = quaternion(1.f, 0.f, 0.f, 0.f),
    const triple& P = triple(0.f),
    const triple& L = triple(0.f),
    double mass = 1.0)
{
    RigidBody b{};
    b.x = x;
    b.q = q;
    b.P = P;
    b.L = L;
    b.mass = mass;
    b.v = triple(0.f);
    b.omega = triple(0.f);
    b.force = triple(0.f);
    b.torque = triple(0.f);
    b.Ibody_inv = glm::mat3(1.0f);
    b.Iinv = glm::mat3(1.0f);
    return b;
}

TEST(DerivFuncTests, StateArray_RoundTrip)
{
    RigidBody src = makeBody(
        /*x*/ triple(1.f, 2.f, 3.f),
        /*q*/ quaternion(1.f, 0.f, 0.f, 0.f),
        /*P*/ triple(4.f, 5.f, 6.f),
        /*L*/ triple(0.1f, 0.2f, 0.3f),
        /*m*/ 2.0);

    std::vector<double> y(13, 0.0);
    StateToArray(src, y, 0);

    RigidBody dst = makeBody();
    dst.mass = 2.0;  // Set mass before ArrayToState
    ArrayToState(y, dst, 0);

    EXPECT_FLOAT_EQ(dst.x.x, 1.f);
    EXPECT_FLOAT_EQ(dst.x.y, 2.f);
    EXPECT_FLOAT_EQ(dst.x.z, 3.f);

    EXPECT_FLOAT_EQ(dst.q.w, 1.f);
    EXPECT_FLOAT_EQ(dst.q.x, 0.f);
    EXPECT_FLOAT_EQ(dst.q.y, 0.f);
    EXPECT_FLOAT_EQ(dst.q.z, 0.f);

    EXPECT_FLOAT_EQ(dst.P.x, 4.f);
    EXPECT_FLOAT_EQ(dst.P.y, 5.f);
    EXPECT_FLOAT_EQ(dst.P.z, 6.f);

    EXPECT_FLOAT_EQ(dst.L.x, 0.1f);
    EXPECT_FLOAT_EQ(dst.L.y, 0.2f);
    EXPECT_FLOAT_EQ(dst.L.z, 0.3f);
}

TEST(DerivFuncTests, StarFunction_ReturnsSkewSymmetric)
{
    std::vector<double> omega = {1.0, 2.0, 3.0};
    glm::mat3 S = Star(omega);

    EXPECT_DOUBLE_EQ(S[0][0],  0.0); EXPECT_DOUBLE_EQ(S[0][1], -3.0); EXPECT_DOUBLE_EQ(S[0][2],  2.0);
    EXPECT_DOUBLE_EQ(S[1][0],  3.0); EXPECT_DOUBLE_EQ(S[1][1],  0.0); EXPECT_DOUBLE_EQ(S[1][2], -1.0);
    EXPECT_DOUBLE_EQ(S[2][0], -2.0); EXPECT_DOUBLE_EQ(S[2][1],  1.0); EXPECT_DOUBLE_EQ(S[2][2],  0.0);
}

TEST(DerivFuncTests, ComputeForceAndTorque_GravityOnly)
{
    RigidBody b = makeBody(/*x*/triple(0), /*q*/quaternion(1,0,0,0),
                           /*P*/triple(0), /*L*/triple(0), /*m*/2.0);

    ComputeForceAndTorque(0.0, b);

    EXPECT_FLOAT_EQ(b.force.x, 0.f);
    EXPECT_FLOAT_EQ(b.force.y, -9.81f * 2.0f);
    EXPECT_FLOAT_EQ(b.force.z, 0.f);

    EXPECT_FLOAT_EQ(b.torque.x, 0.f);
    EXPECT_FLOAT_EQ(b.torque.y, 0.f);
    EXPECT_FLOAT_EQ(b.torque.z, 0.f);
}

TEST(DerivFuncTests, DdtStateToArray_Basic)
{
    RigidBody b = makeBody();
    b.v = triple(5.f, -9.81f, 0.f);
    b.omega = triple(0.f, 0.f, 0.f);
    b.force = triple(0.f, -9.81f, 0.f);
    b.torque = triple(0.f, 0.f, 0.f);

    std::vector<double> xdot(13, 0.0);
    DdtStateToArray(b, xdot, 0);

    EXPECT_DOUBLE_EQ(xdot[0], 5.0);
    EXPECT_NEAR(xdot[1], -9.81, 1e-5);
    EXPECT_DOUBLE_EQ(xdot[2], 0.0);

    EXPECT_DOUBLE_EQ(xdot[3], 0.0);
    EXPECT_DOUBLE_EQ(xdot[4], 0.0);
    EXPECT_DOUBLE_EQ(xdot[5], 0.0);
    EXPECT_DOUBLE_EQ(xdot[6], 0.0);

    EXPECT_DOUBLE_EQ(xdot[7],  0.0);
    EXPECT_NEAR(xdot[8], -9.81, 1e-5);
    EXPECT_DOUBLE_EQ(xdot[9],  0.0);

    EXPECT_DOUBLE_EQ(xdot[10], 0.0);
    EXPECT_DOUBLE_EQ(xdot[11], 0.0);
    EXPECT_DOUBLE_EQ(xdot[12], 0.0);
}

TEST(DerivFuncTests, Dxdt_UsesQuaternionStateFormat)
{
    std::vector<double> state = {
        1.0, 2.0, 3.0,
        1.0, 0.0, 0.0, 0.0,
        5.0, -9.81, 0.0,
        0.1, 0.2, 0.3
    };
    std::vector<double> deriv(13, 0.0);

    Dxdt(0.0, state, deriv);

    ASSERT_EQ(deriv.size(), 13u);
    for (double v : deriv) {
        EXPECT_TRUE(std::isfinite(v));
    }
}

TEST(DerivFuncTests, ArrayToState_ComputesAuxiliaries)
{
    RigidBody src = makeBody(
        /*x*/ triple(0.f),
        /*q*/ quaternion(1.f, 0.f, 0.f, 0.f),
        /*P*/ triple(4.f, 0.f, 0.f),
        /*L*/ triple(0.f, 0.f, 2.f),
        /*m*/ 2.0);
    src.Ibody_inv = glm::mat3(1.0f);

    std::vector<double> y(13, 0.0);
    StateToArray(src, y, 0);

    RigidBody dst = makeBody();
    dst.mass = 2.0;
    dst.Ibody_inv = glm::mat3(1.0f);
    ArrayToState(y, dst, 0);

    EXPECT_FLOAT_EQ(dst.v.x, 2.0f);
    EXPECT_FLOAT_EQ(dst.v.y, 0.0f);
    EXPECT_FLOAT_EQ(dst.v.z, 0.0f);

    EXPECT_FLOAT_EQ(dst.omega.x, 0.0f);
    EXPECT_FLOAT_EQ(dst.omega.y, 0.0f);
    EXPECT_FLOAT_EQ(dst.omega.z, 2.0f);
}

TEST(DerivFuncTests, Star_SizeMismatch_ReturnsZero)
{
    std::vector<double> tooSmall = {1.0, 2.0};
    glm::mat3 S = Star(tooSmall);

    EXPECT_DOUBLE_EQ(S[0][0], 0.0); EXPECT_DOUBLE_EQ(S[0][1], 0.0); EXPECT_DOUBLE_EQ(S[0][2], 0.0);
    EXPECT_DOUBLE_EQ(S[1][0], 0.0); EXPECT_DOUBLE_EQ(S[1][1], 0.0); EXPECT_DOUBLE_EQ(S[1][2], 0.0);
    EXPECT_DOUBLE_EQ(S[2][0], 0.0); EXPECT_DOUBLE_EQ(S[2][1], 0.0); EXPECT_DOUBLE_EQ(S[2][2], 0.0);
}

TEST(DerivFuncTests, DdtStateToArray_QuaternionRateFromOmega)
{
    RigidBody b = makeBody();
    b.q = quaternion(1.f, 0.f, 0.f, 0.f);
    b.omega = triple(0.f, 0.f, 2.f);

    std::vector<double> xdot(13, 0.0);
    DdtStateToArray(b, xdot, 0);

    EXPECT_DOUBLE_EQ(xdot[3], 0.0); // w
    EXPECT_DOUBLE_EQ(xdot[4], 0.0); // x
    EXPECT_DOUBLE_EQ(xdot[5], 0.0); // y
    EXPECT_DOUBLE_EQ(xdot[6], 1.0); // z
}

TEST(DerivFuncTests, DdtStateToArray_PassesForceAndTorque)
{
    RigidBody b = makeBody();
    b.force = triple(1.f, 2.f, 3.f);
    b.torque = triple(-1.f, -2.f, -3.f);

    std::vector<double> xdot(13, 0.0);
    DdtStateToArray(b, xdot, 0);

    EXPECT_DOUBLE_EQ(xdot[7],  1.0);
    EXPECT_DOUBLE_EQ(xdot[8],  2.0);
    EXPECT_DOUBLE_EQ(xdot[9],  3.0);

    EXPECT_DOUBLE_EQ(xdot[10], -1.0);
    EXPECT_DOUBLE_EQ(xdot[11], -2.0);
    EXPECT_DOUBLE_EQ(xdot[12], -3.0);
}

TEST(DerivFuncTests, StateToArray_WithOffset_WritesCorrectSlots)
{
    RigidBody a = makeBody(triple(1.f, 2.f, 3.f));
    RigidBody b = makeBody(triple(4.f, 5.f, 6.f));
    std::vector<double> y(26, 0.0);

    StateToArray(a, y, 0);
    StateToArray(b, y, 13);

    EXPECT_DOUBLE_EQ(y[0], 1.0);
    EXPECT_DOUBLE_EQ(y[1], 2.0);
    EXPECT_DOUBLE_EQ(y[2], 3.0);

    EXPECT_DOUBLE_EQ(y[13], 4.0);
    EXPECT_DOUBLE_EQ(y[14], 5.0);
    EXPECT_DOUBLE_EQ(y[15], 6.0);
}