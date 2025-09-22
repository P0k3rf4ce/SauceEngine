//==============================================================================
// ODESolver.cpp - ODE Solver Implementation for SauceEngine
//==============================================================================

#include "animation/ODEsolver.hpp"
#include <stdexcept>
#include <algorithm>

namespace animation {

//==============================================================================
// EulerSolver Implementation
//==============================================================================

EulerSolver::EulerSolver(double stepSize) : m_stepSize(stepSize) {
    if (stepSize <= 0.0) {
        throw std::invalid_argument("Step size must be positive");
    }
}

void EulerSolver::ode(const std::vector<double>& x0, 
                      std::vector<double>& xEnd, 
                      double t0, 
                      double t1, 
                      DerivFunc dxdt) {
    
    if (x0.empty()) {
        throw std::invalid_argument("Initial state vector cannot be empty");
    }
    
    if (t1 <= t0) {
        throw std::invalid_argument("Final time must be greater than initial time");
    }

    const size_t dim = x0.size();
    xEnd.resize(dim);
    
    // Copy initial conditions
    std::vector<double> x_current = x0;
    std::vector<double> xdot(dim);
    
    double t_current = t0;
    
    // Integrate using Euler's method
    while (t_current < t1) {
        // Adjust step size if we would overshoot the final time
        double h = std::min(m_stepSize, t1 - t_current);
        
        // Compute derivatives at current state
        dxdt(t_current, x_current, xdot);
        
        // Euler step: x_new = x_old + h * dx/dt
        for (size_t i = 0; i < dim; ++i) {
            x_current[i] += h * xdot[i];
        }
        
        t_current += h;
    }
    
    // Copy final result
    xEnd = x_current;
}

void EulerSolver::setStepSize(double stepSize) {
    if (stepSize <= 0.0) {
        throw std::invalid_argument("Step size must be positive");
    }
    m_stepSize = stepSize;
}

double EulerSolver::getStepSize() const {
    return m_stepSize;
}

//==============================================================================
// Factory Function Implementation
//==============================================================================

std::unique_ptr<ODESolver> createODESolver(const std::string& solverType, double stepSize) {
    if (solverType == "euler") {
        return std::make_unique<EulerSolver>(stepSize);
    }
    // Future solver types can be added here
    throw std::invalid_argument("Unknown solver type: " + solverType);
}

} // namespace animation