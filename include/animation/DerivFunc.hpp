#ifndef ANIMATION_DERIVFUNC_HPP
#define ANIMATION_DERIVFUNC_HPP

#include <vector>
#include <Eigen/Geometry>
#include <cmath>

namespace animation {

/**
 * @brief Function pointer type for derivative functions in ODE systems
 * 
 * This typedef defines the signature for functions that compute derivatives
 * in ordinary differential equation systems of the form dx/dt = f(t, x).
 * Following the format from Pixar's "Physically Based Modeling" section 3.
 * 
 * @param t Current time value
 * @param x Current state vector array
 * @param xdot Output vector for computed derivatives (dx/dt)
 */
typedef void (*DerivFunc)(double t, const std::vector<double> &x, std::vector<double> &xdot);

/**
 * @brief Main derivative function for rigid body simulation (Section 3 format)
 * 
 * Implements Dxdt() function from Pixar paper section 3.
 * State vector format (18 elements per rigid body):
 * [x, y, z,                    // position (3 elements)
 *  R11, R12, R13, R21, R22, R23, R31, R32, R33,  // rotation matrix (9 elements)
 *  Px, Py, Pz,                 // linear momentum (3 elements)
 *  Lx, Ly, Lz]                 // angular momentum (3 elements)
 */
void Dxdt(double t, const std::vector<double> &x, std::vector<double> &xdot);

// Helper functions (following Pixar paper structure)

/**
 * @brief Copy state information from rigid body struct to array
 * Equivalent to StateToArray() in section 3
 */
void StateToArray(const std::vector<double> &rigidBodyState, std::vector<double> &y, int offset = 0);

/**
 * @brief Copy information from array into state variables
 * Equivalent to ArrayToState() in section 3
 */
void ArrayToState(const std::vector<double> &y, std::vector<double> &rigidBodyState, int offset = 0);

/**
 * @brief Compute d/dt X(t) for a single rigid body
 * Equivalent to DdtStateToArray() in section 3
 */
void DdtStateToArray(const std::vector<double> &rigidBodyState, std::vector<double> &xdot, int offset = 0);

/**
 * @brief Compute force and torque for a rigid body
 * Equivalent to ComputeForceAndTorque() in section 3
 */
void ComputeForceAndTorque(double t, std::vector<double> &rigidBodyState);

/**
 * @brief Create skew-symmetric matrix from vector
 * Equivalent to Star() function in section 3
 */
void Star(const std::vector<double> &omega, std::vector<double> &omegaStar);

} // namespace animation

#endif