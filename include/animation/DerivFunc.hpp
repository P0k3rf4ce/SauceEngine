#ifndef ANIMATION_DERIVFUNC_HPP
#define ANIMATION_DERIVFUNC_HPP

#include <vector>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include "CollisionDetection.hpp"

namespace animation {

// Note: DerivFunc is already defined in ODEsolver.hpp
// Do NOT redefine it here to avoid conflicts

/**
 * @brief Main derivative function for rigid body simulation
 * 
 * State vector format (13 elements per rigid body):
 * [x, y, z,           // position (3 elements)
 *  q.w, q.x, q.y, q.z, // rotation quaternion (4 elements)
 *  Px, Py, Pz,        // linear momentum (3 elements)
 *  Lx, Ly, Lz]        // angular momentum (3 elements)
 */
void Dxdt(double t, const std::vector<double> &x, std::vector<double> &xdot);

// Helper functions

/**
 * @brief Copy state information from rigid body struct to array
 */
void StateToArray(const RigidBody &rigidBodyState, std::vector<double> &y, int offset = 0);

/**
 * @brief Copy information from array into state variables
 */
void ArrayToState(const std::vector<double> &y, RigidBody &rigidBodyState, int offset = 0);

/**
 * @brief Compute d/dt X(t) for a single rigid body
 */
void DdtStateToArray(const RigidBody &rigidBodyState, std::vector<double> &xdot, int offset = 0);

/**
 * @brief Compute force and torque for a rigid body
 */
void ComputeForceAndTorque(double t, RigidBody &rigidBodyState);

/**
 * @brief Create skew-symmetric matrix from vector
 */
glm::mat3 Star(const std::vector<double> &omega);

} // namespace animation

#endif