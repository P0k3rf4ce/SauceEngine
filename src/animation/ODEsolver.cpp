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

double EulerSolver::ode(const std::vector<double>& x0, 
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
    
    std::vector<double> x_current = x0;
    std::vector<double> xdot(dim);
    
    double t_current = t0;
    
    const double eps = 1e-14;
    while (t_current + m_stepSize <= t1 + eps) {
        dxdt(t_current, x_current, xdot);
        
        for (size_t i = 0; i < dim; ++i) {
            x_current[i] += m_stepSize * xdot[i];
        }
        
        t_current += m_stepSize;
    }
    
    xEnd = x_current;
    return t1 - t_current;
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
// RK4Solver Implementation (Runge-Kutta 4th Order)
//==============================================================================

RK4Solver::RK4Solver(double stepSize) : m_stepSize(stepSize) {
    if (stepSize <= 0.0) {
        throw std::invalid_argument("Step size must be positive");
    }
}

double RK4Solver::ode(const std::vector<double>& x0, 
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
    
    std::vector<double> x_current = x0;
    std::vector<double> k1(dim), k2(dim), k3(dim), k4(dim);
    std::vector<double> x_temp(dim);
    
    double t_current = t0;
    const double eps = 1e-14;
    
    while (t_current + m_stepSize <= t1 + eps) {
        // k1 = f(t, x)
        dxdt(t_current, x_current, k1);
        
        // k2 = f(t + h/2, x + h*k1/2)
        for (size_t i = 0; i < dim; ++i) {
            x_temp[i] = x_current[i] + 0.5 * m_stepSize * k1[i];
        }
        dxdt(t_current + 0.5 * m_stepSize, x_temp, k2);
        
        // k3 = f(t + h/2, x + h*k2/2)
        for (size_t i = 0; i < dim; ++i) {
            x_temp[i] = x_current[i] + 0.5 * m_stepSize * k2[i];
        }
        dxdt(t_current + 0.5 * m_stepSize, x_temp, k3);
        
        // k4 = f(t + h, x + h*k3)
        for (size_t i = 0; i < dim; ++i) {
            x_temp[i] = x_current[i] + m_stepSize * k3[i];
        }
        dxdt(t_current + m_stepSize, x_temp, k4);
        
        // x_new = x + h/6 * (k1 + 2*k2 + 2*k3 + k4)
        for (size_t i = 0; i < dim; ++i) {
            x_current[i] += (m_stepSize / 6.0) * (k1[i] + 2.0*k2[i] + 2.0*k3[i] + k4[i]);
        }
        
        t_current += m_stepSize;
    }
    
    xEnd = x_current;
    return t1 - t_current;
}

void RK4Solver::setStepSize(double stepSize) {
    if (stepSize <= 0.0) {
        throw std::invalid_argument("Step size must be positive");
    }
    m_stepSize = stepSize;
}

double RK4Solver::getStepSize() const {
    return m_stepSize;
}

//==============================================================================
// Factory Function Implementation
//==============================================================================

std::unique_ptr<ODESolver> createODESolver(const std::string& solverType, double stepSize) {
    if (solverType == "euler") {
        return std::make_unique<EulerSolver>(stepSize);
    } else if (solverType == "rk4") {
        return std::make_unique<RK4Solver>(stepSize);
    }
    throw std::invalid_argument("Unknown solver type: " + solverType);
}

} // namespace animation