//==============================================================================
// ODESolver.h - ODE Solver Interface and Euler Implementation for SauceEngine
//==============================================================================

#ifndef SAUCE_ENGINE_ODE_SOLVER_H
#define SAUCE_ENGINE_ODE_SOLVER_H

#include <vector>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <algorithm>

namespace SauceEngine {

/**
 * @brief Function pointer type for derivative functions in ODE systems
 * 
 * This typedef defines the signature for functions that compute derivatives
 * in ordinary differential equation systems of the form dx/dt = f(t, x).
 * 
 * @param t Current time value
 * @param x Current state vector
 * @param xdot Output vector for computed derivatives (dx/dt)
 */
typedef void (*DerivFunc)(double t, const std::vector<double>& x, std::vector<double>& xdot);

/**
 * @brief Abstract base class for ODE solvers
 * 
 * This class defines the interface for numerical ODE solvers in SauceEngine.
 * Concrete implementations should inherit from this class and implement the
 * solve method using specific numerical integration algorithms.
 */
class ODESolver {
public:
    virtual ~ODESolver() = default;

    /**
     * @brief Solve an ODE system from initial to final time
     * 
     * Integrates the ODE system dx/dt = f(t, x) from time t0 to t1,
     * starting with initial conditions x0 and storing the final result in xEnd.
     * 
     * @param x0 Initial state vector at time t0
     * @param xEnd Output vector for final state at time t1
     * @param t0 Initial time
     * @param t1 Final time
     * @param dxdt Derivative function defining the ODE system
     */
    virtual void ode(const std::vector<double>& x0, 
                     std::vector<double>& xEnd, 
                     double t0, 
                     double t1, 
                     DerivFunc dxdt) = 0;

    /**
     * @brief Set the step size for the numerical integration
     * @param stepSize Integration step size (must be positive)
     */
    virtual void setStepSize(double stepSize) = 0;

    /**
     * @brief Get the current step size
     * @return Current integration step size
     */
    virtual double getStepSize() const = 0;
};

/**
 * @brief Euler method implementation for solving ODEs
 * 
 * This class implements the simplest numerical integration method for ODEs,
 * known as Euler's method or the forward Euler method. While not the most
 * accurate method, it's stable and easy to understand.
 * 
 * The method approximates the solution using:
 * x(t + h) ≈ x(t) + h * f(t, x(t))
 * 
 * where h is the step size and f is the derivative function.
 */
class EulerSolver : public ODESolver {
private:
    double m_stepSize;  ///< Integration step size

public:
    /**
     * @brief Constructor with default step size
     * @param stepSize Integration step size (default: 0.01)
     */
    explicit EulerSolver(double stepSize = 0.01) : m_stepSize(stepSize) {
        if (stepSize <= 0.0) {
            throw std::invalid_argument("Step size must be positive");
        }
    }

    /**
     * @brief Solve ODE using Euler's method
     * 
     * Implements the forward Euler method to integrate the ODE system
     * from t0 to t1. The method subdivides the time interval into steps
     * of size m_stepSize and applies the Euler formula at each step.
     * 
     * @param x0 Initial state vector
     * @param xEnd Output vector for final state
     * @param t0 Initial time
     * @param t1 Final time  
     * @param dxdt Derivative function
     */
    void ode(const std::vector<double>& x0, 
             std::vector<double>& xEnd, 
             double t0, 
             double t1, 
             DerivFunc dxdt) override {
        
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

    /**
     * @brief Set the integration step size
     * @param stepSize New step size (must be positive)
     */
    void setStepSize(double stepSize) override {
        if (stepSize <= 0.0) {
            throw std::invalid_argument("Step size must be positive");
        }
        m_stepSize = stepSize;
    }

    /**
     * @brief Get the current step size
     * @return Current integration step size
     */
    double getStepSize() const override {
        return m_stepSize;
    }
};

/**
 * @brief Factory function to create ODE solver instances
 * @param solverType Type of solver ("euler", "rk4", etc.)
 * @param stepSize Integration step size
 * @return Unique pointer to ODE solver instance
 */
inline std::unique_ptr<ODESolver> createODESolver(const std::string& solverType, double stepSize = 0.01) {
    if (solverType == "euler") {
        return std::make_unique<EulerSolver>(stepSize);
    }
    // Future solver types can be added here
    throw std::invalid_argument("Unknown solver type: " + solverType);
}

} // namespace SauceEngine

#endif // SAUCE_ENGINE_ODE_SOLVER_H