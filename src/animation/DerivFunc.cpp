#define GLM_ENABLE_EXPERIMENTAL
#include "animation/DerivFunc.hpp"
#include "animation/CollisionDetection.hpp"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace animation {

// Constants following section 3
static constexpr int STATE_SIZE = 13;

/**
 * @brief Copy state information from rigid body to array
 */
void StateToArray(const RigidBody& rigidBodyState, std::vector<double> &y, int offset) {
    if (y.size() < static_cast<size_t>(offset + STATE_SIZE)) {
        y.resize(offset + STATE_SIZE);
    }
    
    int idx = offset;
    
    // Copy position: x, y, z
    y[idx++] = rigidBodyState.x.x;
    y[idx++] = rigidBodyState.x.y;
    y[idx++] = rigidBodyState.x.z;

    // Copy rotation quaternion q (w, x, y, z)
    y[idx++] = rigidBodyState.q.w;
    y[idx++] = rigidBodyState.q.x;
    y[idx++] = rigidBodyState.q.y;
    y[idx++] = rigidBodyState.q.z;

    // Copy linear momentum P
    y[idx++] = rigidBodyState.P.x;
    y[idx++] = rigidBodyState.P.y;
    y[idx++] = rigidBodyState.P.z;

    // Copy angular momentum L
    y[idx++] = rigidBodyState.L.x;
    y[idx++] = rigidBodyState.L.y;
    y[idx++] = rigidBodyState.L.z;
}

/**
 * @brief Copy information from array into state variables
 * Note: rigidBodyState.mass and rigidBodyState.Ibody_inv must be set before calling this
 */
void ArrayToState(const std::vector<double> &y, RigidBody& rigidBodyState, int offset) {
    int idx = offset;
    
    // Copy position
    rigidBodyState.x = triple(
        static_cast<float>(y[idx]),
        static_cast<float>(y[idx + 1]),
        static_cast<float>(y[idx + 2])
    );
    idx += 3;

    // Copy rotation quaternion q
    rigidBodyState.q = quaternion(
        static_cast<float>(y[idx]),     // w
        static_cast<float>(y[idx + 1]), // x
        static_cast<float>(y[idx + 2]), // y
        static_cast<float>(y[idx + 3])  // z
    );
    idx += 4;

    // Copy linear momentum P
    rigidBodyState.P = triple(
        static_cast<float>(y[idx]),
        static_cast<float>(y[idx + 1]),
        static_cast<float>(y[idx + 2])
    );
    idx += 3;

    // Copy angular momentum L
    rigidBodyState.L = triple(
        static_cast<float>(y[idx]),
        static_cast<float>(y[idx + 1]),
        static_cast<float>(y[idx + 2])
    );
    idx += 3;

    // Compute auxiliary variables
    glm::mat3 R = glm::toMat3(rigidBodyState.q);
    rigidBodyState.v = rigidBodyState.P / static_cast<float>(rigidBodyState.mass);
    rigidBodyState.Iinv = R * rigidBodyState.Ibody_inv * glm::transpose(R);
    rigidBodyState.omega = rigidBodyState.Iinv * rigidBodyState.L;
}

/**
 * @brief Compute force and torque for a rigid body
 */
void ComputeForceAndTorque(double t, RigidBody& rigidBodyState) {
    // Simple example: gravity and no torque
    rigidBodyState.force = triple(0.0f, -9.81f * static_cast<float>(rigidBodyState.mass), 0.0f);
    rigidBodyState.torque = triple(0.0f, 0.0f, 0.0f);
}

/**
 * @brief Create skew-symmetric matrix from vector
 */
glm::mat3 Star(const std::vector<double> &omega) {
    if (omega.size() < 3) return glm::mat3(0.0f);

    float ax = static_cast<float>(omega[0]);
    float ay = static_cast<float>(omega[1]);
    float az = static_cast<float>(omega[2]);
    
    glm::mat3 omegaStar(0.0f);
    omegaStar[0][0] =  0.0f; omegaStar[0][1] = -az;   omegaStar[0][2] =  ay;
    omegaStar[1][0] =  az;   omegaStar[1][1] =  0.0f; omegaStar[1][2] = -ax;
    omegaStar[2][0] = -ay;   omegaStar[2][1] =  ax;   omegaStar[2][2] =  0.0f;
    return omegaStar;
}

/**
 * @brief Compute d/dt X(t) for a single rigid body
 */
void DdtStateToArray(const RigidBody& rigidBodyState, std::vector<double> &xdot, int offset) {
    if (xdot.size() < static_cast<size_t>(offset + STATE_SIZE)) {
        xdot.resize(offset + STATE_SIZE);
    }

    int idx = offset;
    
    // d/dt x = v
    xdot[idx++] = rigidBodyState.v.x;
    xdot[idx++] = rigidBodyState.v.y;
    xdot[idx++] = rigidBodyState.v.z;

    // d/dt q = 0.5 * (0, omega) * q
    quaternion omegaQuat(0.0f, rigidBodyState.omega.x, rigidBodyState.omega.y, rigidBodyState.omega.z);
    quaternion qdot = 0.5f * (omegaQuat * rigidBodyState.q);
    xdot[idx++] = qdot.w;
    xdot[idx++] = qdot.x;
    xdot[idx++] = qdot.y;
    xdot[idx++] = qdot.z;

    // d/dt P = force
    xdot[idx++] = rigidBodyState.force.x;
    xdot[idx++] = rigidBodyState.force.y;
    xdot[idx++] = rigidBodyState.force.z;

    // d/dt L = torque
    xdot[idx++] = rigidBodyState.torque.x;
    xdot[idx++] = rigidBodyState.torque.y;
    xdot[idx++] = rigidBodyState.torque.z;
}

/**
 * @brief Main derivative function for rigid body simulation
 * 
 * Note: This uses default mass=1.0 and identity inertia tensor.
 * For custom mass/inertia, use ArrayToState directly with a pre-configured RigidBody.
 */
void Dxdt(double t, const std::vector<double> &x, std::vector<double> &xdot) {
    RigidBody body;
    // Set default physical properties
    body.mass = 1.0;
    body.Ibody_inv = glm::mat3(1.0f); // Identity matrix (unit inertia)
    
    // Deserialize state
    ArrayToState(x, body, 0);
    
    // Compute forces
    ComputeForceAndTorque(t, body);
    
    // Serialize derivative
    DdtStateToArray(body, xdot, 0);
}

} // namespace animation
