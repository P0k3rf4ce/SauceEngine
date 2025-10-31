#include "animation/DerivFunc.hpp"
#include <cmath>

namespace animation {

// Constants following section 3
const int STATE_SIZE = 18;  // 3 + 9 + 3 + 3 = 18 elements per rigid body

/**
 * @brief Copy state information from rigid body to array
 * 
 * Following StateToArray() from section 3 of Pixar paper.
 * Copies position, rotation matrix, linear momentum, and angular momentum.
 */
void StateToArray(const std::vector<double> &rigidBodyState, std::vector<double> &y, int offset) {
    if (rigidBodyState.size() < STATE_SIZE) return;
    
    int idx = offset;
    
    // Copy position: x, y, z
    y[idx++] = rigidBodyState[0];  // x
    y[idx++] = rigidBodyState[1];  // y  
    y[idx++] = rigidBodyState[2];  // z
    
    // Copy rotation matrix R (3x3 = 9 elements)
    for (int i = 0; i < 9; i++) {
        y[idx++] = rigidBodyState[3 + i];
    }
    
    // Copy linear momentum P
    y[idx++] = rigidBodyState[12];  // Px
    y[idx++] = rigidBodyState[13];  // Py
    y[idx++] = rigidBodyState[14];  // Pz
    
    // Copy angular momentum L
    y[idx++] = rigidBodyState[15];  // Lx
    y[idx++] = rigidBodyState[16];  // Ly
    y[idx++] = rigidBodyState[17];  // Lz
}

/**
 * @brief Copy information from array into state variables
 * 
 * Following ArrayToState() from section 3 of Pixar paper.
 * Also computes auxiliary variables v(t) and ω(t).
 */
void ArrayToState(const std::vector<double> &y, std::vector<double> &rigidBodyState, int offset) {
    if (rigidBodyState.size() < STATE_SIZE) rigidBodyState.resize(STATE_SIZE);
    
    int idx = offset;
    
    // Copy position
    rigidBodyState[0] = y[idx++];  // x
    rigidBodyState[1] = y[idx++];  // y
    rigidBodyState[2] = y[idx++];  // z
    
    // Copy rotation matrix R
    for (int i = 0; i < 9; i++) {
        rigidBodyState[3 + i] = y[idx++];
    }
    
    // Copy linear momentum P
    rigidBodyState[12] = y[idx++];  // Px
    rigidBodyState[13] = y[idx++];  // Py  
    rigidBodyState[14] = y[idx++];  // Pz
    
    // Copy angular momentum L
    rigidBodyState[15] = y[idx++];  // Lx
    rigidBodyState[16] = y[idx++];  // Ly
    rigidBodyState[17] = y[idx++];  // Lz
    
    // Note: Auxiliary variables v(t) = P(t)/M and ω(t) = I^(-1)(t)L(t)
    // would be computed here in a full implementation
}

/**
 * @brief Compute force and torque for a rigid body
 * 
 * Following ComputeForceAndTorque() from section 3.
 * Computes forces and torques acting on the rigid body at time t.
 */
void ComputeForceAndTorque(double t, std::vector<double> &rigidBodyState) {
    if (rigidBodyState.size() < 24) rigidBodyState.resize(24);
    // Simple example: gravity and no torque
    // In a real implementation, this would compute all forces and torques:
    // gravity, wind, interaction with other bodies, etc.
    
    // Forces are stored temporarily (not part of state vector)
    // For this implementation, we'll use simple constant forces
    // F = [0, -9.81, 0] (gravity in y direction)
    // τ = [0, 0, 0] (no torque)
    rigidBodyState[18] = 0.0;      // Fx
    rigidBodyState[19] = -9.81;    // Fy
    rigidBodyState[20] = 0.0;      // Fz

    rigidBodyState[21] = 0.0;      // τx
    rigidBodyState[22] = 0.0;      // τy
    rigidBodyState[23] = 0.0;      // τz
}

/**
 * @brief Create skew-symmetric matrix from vector
 * 
 * Following Star() function from section 3.
 * Returns the 3x3 skew-symmetric matrix:
 * [  0   -az   ay ]
 * [ az    0  -ax ]
 * [-ay   ax    0 ]
 */
void Star(const std::vector<double> &omega, std::vector<double> &omegaStar) {
    if (omega.size() < 3 || omegaStar.size() < 9) return;
    
    double ax = omega[0], ay = omega[1], az = omega[2];
    
    // Row-major order for 3x3 matrix
    omegaStar[0] =  0.0;  omegaStar[1] = -az;   omegaStar[2] =  ay;   // Row 1
    omegaStar[3] =  az;   omegaStar[4] =  0.0;  omegaStar[5] = -ax;   // Row 2  
    omegaStar[6] = -ay;   omegaStar[7] =  ax;   omegaStar[8] =  0.0;  // Row 3
}

/**
 * @brief Compute d/dt X(t) for a single rigid body
 * 
 * Following DdtStateToArray() from section 3.
 * This is the core function that computes derivatives.
 */
void DdtStateToArray(const std::vector<double> &rigidBodyState, std::vector<double> &xdot, int offset) {
    if (rigidBodyState.size() < STATE_SIZE) return;
    
    // Constants
    const double mass = 1.0;
    const double Ibody_inv[9] = {1.0, 0.0, 0.0,  // Simplified: identity matrix
                                 0.0, 1.0, 0.0,
                                 0.0, 0.0, 1.0};
    
    // Extract state variables
    double x = rigidBodyState[0], y = rigidBodyState[1], z = rigidBodyState[2];
    double Px = rigidBodyState[12], Py = rigidBodyState[13], Pz = rigidBodyState[14];
    double Lx = rigidBodyState[15], Ly = rigidBodyState[16], Lz = rigidBodyState[17];
    
    // Compute auxiliary variables
    // v(t) = P(t)/M
    double vx = Px / mass, vy = Py / mass, vz = Pz / mass;
    
    // ω(t) = I^(-1)(t)L(t) (simplified with identity inertia)
    double omega_x = Lx, omega_y = Ly, omega_z = Lz;
    
    int idx = offset;
    
    // Copy d/dt x(t) = v(t) into xdot
    xdot[idx++] = vx;
    xdot[idx++] = vy;
    xdot[idx++] = vz;
    
    // Compute dR/dt = ω* × R
    std::vector<double> omega = {omega_x, omega_y, omega_z};
    std::vector<double> omegaStar(9);
    Star(omega, omegaStar);
    
    // Multiply ω* × R and store in xdot
    for (int i = 0; i < 3; i++) {        // For each row of result
        for (int j = 0; j < 3; j++) {    // For each column of result
            double sum = 0.0;
            for (int k = 0; k < 3; k++) {  // Dot product
                sum += omegaStar[i*3 + k] * rigidBodyState[3 + k*3 + j];
            }
            xdot[idx++] = sum;
        }
    }
    
    // dP/dt = F(t) and dL/dt = τ(t)
    // Check if forces and torques are already computed (extended state vector)
    // or need to be computed directly
    double fx, fy, fz, tau_x, tau_y, tau_z;
    
    if (rigidBodyState.size() >= 24) {
        // Use precomputed forces and torques
        fx = rigidBodyState[18];
        fy = rigidBodyState[19]; 
        fz = rigidBodyState[20];
        tau_x = rigidBodyState[21];
        tau_y = rigidBodyState[22];
        tau_z = rigidBodyState[23];
    } else {
        // Compute forces and torques directly
        fx = 0.0;      // force in x
        fy = -9.81;    // gravity in y
        fz = 0.0;      // force in z
        tau_x = 0.0;   // torque about x
        tau_y = 0.0;   // torque about y  
        tau_z = 0.0;   // torque about z
    }
    
    // dP/dt = F(t) 
    xdot[idx++] = fx;    // dPx/dt = Fx
    xdot[idx++] = fy;    // dPy/dt = Fy
    xdot[idx++] = fz;    // dPz/dt = Fz

    // dL/dt = τ(t)
    xdot[idx++] = tau_x; // dLx/dt = τx
    xdot[idx++] = tau_y; // dLy/dt = τy
    xdot[idx++] = tau_z; // dLz/dt = τz
}

/**
 * @brief Main derivative function (Section 3 format)
 * 
 * Following Dxdt() from section 3 of Pixar paper.
 * This is the function called by the ODE solver.
 */
void Dxdt(double t, const std::vector<double> &x, std::vector<double> &xdot) {
    // Assume single rigid body for now (can be extended to multiple bodies)
    const int NBODIES = 1;
    
    if (x.size() < STATE_SIZE * NBODIES || xdot.size() < STATE_SIZE * NBODIES) {
        return;
    }
    
    // Process each rigid body
    for (int i = 0; i < NBODIES; i++) {
        // Extract state for this body
        std::vector<double> bodyState(STATE_SIZE);
        ArrayToState(x, bodyState, i * STATE_SIZE);
        
        // Compute forces and torques
        ComputeForceAndTorque(t, bodyState);
        
        // Compute derivatives
        DdtStateToArray(bodyState, xdot, i * STATE_SIZE);
    }
}

} // namespace animation
