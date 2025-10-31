#include <gtest/gtest.h>
#include <vector>
#include "animation/DerivFunc.hpp"
#include <cmath>

using namespace animation;

TEST(DerivFuncTests, PixarSection3Format) {
    /**
     * @brief Test the main Dxdt function following Pixar section 3 format
     * 
     * State vector format (18 elements per rigid body):
     * [x, y, z,                    // position (3)
     *  R11, R12, R13, R21, R22, R23, R31, R32, R33,  // rotation matrix (9)
     *  Px, Py, Pz,                 // linear momentum (3)
     *  Lx, Ly, Lz]                 // angular momentum (3)
     */
    
    // Initialize state with identity rotation matrix and some momentum
    std::vector<double> state = {
        1.0, 2.0, 3.0,              // position
        1.0, 0.0, 0.0,               // R row 1 (identity matrix)
        0.0, 1.0, 0.0,               // R row 2
        0.0, 0.0, 1.0,               // R row 3
        5.0, -9.81, 0.0,             // linear momentum P
        0.1, 0.2, 0.3                // angular momentum L
    };
    std::vector<double> deriv(18);
    
    Dxdt(0.0, state, deriv);
    
    // Position derivatives = P/m (mass = 1.0)
    EXPECT_DOUBLE_EQ(deriv[0], 5.0);     // dx/dt = Px/m
    EXPECT_DOUBLE_EQ(deriv[1], -9.81);   // dy/dt = Py/m
    EXPECT_DOUBLE_EQ(deriv[2], 0.0);     // dz/dt = Pz/m
    
    // // Linear momentum derivatives = F
    // EXPECT_DOUBLE_EQ(deriv[12], 0.0);    // dPx/dt = Fx = 0
    // EXPECT_DOUBLE_EQ(deriv[13], -9.81);  // dPy/dt = Fy = -9.81 (gravity)
    // EXPECT_DOUBLE_EQ(deriv[14], 0.0);    // dPz/dt = Fz = 0
    
    // // Angular momentum derivatives = τ
    // EXPECT_DOUBLE_EQ(deriv[15], 0.0);    // dLx/dt = τx = 0
    // EXPECT_DOUBLE_EQ(deriv[16], 0.0);    // dLy/dt = τy = 0
    // EXPECT_DOUBLE_EQ(deriv[17], 0.0);    // dLz/dt = τz = 0
}

TEST(DerivFuncTests, StateToArrayFunction) {
    /**
     * @brief Test StateToArray helper function
     */
    
    std::vector<double> rigidBodyState = {
        1.0, 2.0, 3.0,              // position
        1.0, 0.0, 0.0,               // R row 1
        0.0, 1.0, 0.0,               // R row 2
        0.0, 0.0, 1.0,               // R row 3
        4.0, 5.0, 6.0,               // momentum
        0.1, 0.2, 0.3                // angular momentum
    };
    
    std::vector<double> array(18);
    StateToArray(rigidBodyState, array, 0);
    
    // Verify position
    EXPECT_DOUBLE_EQ(array[0], 1.0);
    EXPECT_DOUBLE_EQ(array[1], 2.0);
    EXPECT_DOUBLE_EQ(array[2], 3.0);
    
    // Verify identity matrix
    EXPECT_DOUBLE_EQ(array[3], 1.0);   // R11
    EXPECT_DOUBLE_EQ(array[4], 0.0);   // R12
    EXPECT_DOUBLE_EQ(array[5], 0.0);   // R13
    
    // Verify momentum
    EXPECT_DOUBLE_EQ(array[12], 4.0);  // Px
    EXPECT_DOUBLE_EQ(array[13], 5.0);  // Py
    EXPECT_DOUBLE_EQ(array[14], 6.0);  // Pz
}

TEST(DerivFuncTests, ArrayToStateFunction) {
    /**
     * @brief Test ArrayToState helper function
     */
    
    std::vector<double> array = {
        1.0, 2.0, 3.0,              // position
        1.0, 0.0, 0.0,               // R row 1
        0.0, 1.0, 0.0,               // R row 2
        0.0, 0.0, 1.0,               // R row 3
        4.0, 5.0, 6.0,               // momentum
        0.1, 0.2, 0.3                // angular momentum
    };
    
    std::vector<double> rigidBodyState(18);
    ArrayToState(array, rigidBodyState, 0);
    
    // Verify position
    EXPECT_DOUBLE_EQ(rigidBodyState[0], 1.0);
    EXPECT_DOUBLE_EQ(rigidBodyState[1], 2.0);
    EXPECT_DOUBLE_EQ(rigidBodyState[2], 3.0);
    
    // Verify momentum
    EXPECT_DOUBLE_EQ(rigidBodyState[12], 4.0);
    EXPECT_DOUBLE_EQ(rigidBodyState[13], 5.0);
    EXPECT_DOUBLE_EQ(rigidBodyState[14], 6.0);
}

TEST(DerivFuncTests, StarFunction) {
    /**
     * @brief Test Star (skew-symmetric matrix) function
     */
    
    std::vector<double> omega = {1.0, 2.0, 3.0};
    std::vector<double> omegaStar(9);
    
    Star(omega, omegaStar);
    
    // Verify skew-symmetric matrix structure
    // [  0  -3   2 ]
    // [  3   0  -1 ]
    // [ -2   1   0 ]
    
    EXPECT_DOUBLE_EQ(omegaStar[0],  0.0);  // [0,0]
    EXPECT_DOUBLE_EQ(omegaStar[1], -3.0);  // [0,1]
    EXPECT_DOUBLE_EQ(omegaStar[2],  2.0);  // [0,2]
    
    EXPECT_DOUBLE_EQ(omegaStar[3],  3.0);  // [1,0]
    EXPECT_DOUBLE_EQ(omegaStar[4],  0.0);  // [1,1]
    EXPECT_DOUBLE_EQ(omegaStar[5], -1.0);  // [1,2]
    
    EXPECT_DOUBLE_EQ(omegaStar[6], -2.0);  // [2,0]
    EXPECT_DOUBLE_EQ(omegaStar[7],  1.0);  // [2,1]
    EXPECT_DOUBLE_EQ(omegaStar[8],  0.0);  // [2,2]
}

TEST(DerivFuncTests, ComputeForceAndTorqueFunction) {
    /**
     * @brief Test ComputeForceAndTorque helper function
     */
    
    std::vector<double> rigidBodyState = {
        1.0, 2.0, 3.0,              // position
        1.0, 0.0, 0.0,               // R row 1
        0.0, 1.0, 0.0,               // R row 2
        0.0, 0.0, 1.0,               // R row 3
        5.0, -9.81, 0.0,             // momentum
        0.0, 0.0, 0.0                // angular momentum (no rotation)
    };
    
    ComputeForceAndTorque(0.0, rigidBodyState);
    
    // After computation, force should reflect gravity
    // Force = [0, -9.81, 0]
    EXPECT_DOUBLE_EQ(rigidBodyState[18], 0.0);      // Fx
    EXPECT_DOUBLE_EQ(rigidBodyState[19], -9.81);    // Fy
    EXPECT_DOUBLE_EQ(rigidBodyState[20], 0.0);      // Fz
    
    // Torque should be zero in this simple case
    EXPECT_DOUBLE_EQ(rigidBodyState[21], 0.0);      // τx
    EXPECT_DOUBLE_EQ(rigidBodyState[22], 0.0);      // τy
    EXPECT_DOUBLE_EQ(rigidBodyState[23], 0.0);      // τz
}

TEST(DerivFuncTests, DdtStateToArrayFunction) {
    /**
     * @brief Test DdtStateToArray helper function
     */
    
    std::vector<double> rigidBodyState = {
        1.0, 2.0, 3.0,              // position
        1.0, 0.0, 0.0,               // R row 1 (identity)
        0.0, 1.0, 0.0,               // R row 2
        0.0, 0.0, 1.0,               // R row 3
        5.0, -9.81, 0.0,             // momentum
        0.0, 0.0, 0.0                // angular momentum (no rotation)
    };
    
    std::vector<double> xdot(18);
    DdtStateToArray(rigidBodyState, xdot, 0);
    
    // Position derivatives = P/m
    EXPECT_DOUBLE_EQ(xdot[0], 5.0);      // dx/dt = Px/m
    EXPECT_DOUBLE_EQ(xdot[1], -9.81);    // dy/dt = Py/m
    EXPECT_DOUBLE_EQ(xdot[2], 0.0);      // dz/dt = Pz/m
    
    // Force derivatives = F
    EXPECT_DOUBLE_EQ(xdot[12], 0.0);     // dPx/d
    EXPECT_DOUBLE_EQ(xdot[13], -9.81);   // dPy/dt
    EXPECT_DOUBLE_EQ(xdot[14], 0.0);     // dPz/dt

    // Torque derivatives = τ
    EXPECT_DOUBLE_EQ(xdot[15], 0.0);     // dLx/dt
    EXPECT_DOUBLE_EQ(xdot[16], 0.0);     // dLy/dt
    EXPECT_DOUBLE_EQ(xdot[17], 0.0);     // dLz/dt
}