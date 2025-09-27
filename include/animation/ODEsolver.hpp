//==============================================================================
// ODESolver.hpp - ODE Solver Interface and Declarations for SauceEngine
//==============================================================================

#ifndef ODE_SOLVER_HPP
#define ODE_SOLVER_HPP

#include <vector>
#include <memory>
#include <string>

namespace animation {

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
     * @param xEnd Output vector for final state at time reached
     * @param t0 Initial time
     * @param t1 Target final time
     * @param dxdt Derivative function defining the ODE system
     * @return Time remaining (t1 - t_final_reached)
     */
    virtual double ode(const std::vector<double>& x0, 
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
    explicit EulerSolver(double stepSize = 0.01);

    /**
     * @brief Solve ODE using Euler's method
     * 
     * Implements the forward Euler method to integrate the ODE system
     * from t0 to t1. The method subdivides the time interval into steps
     * of size m_stepSize and applies the Euler formula at each step.
     * 
     * @param x0 Initial state vector
     * @param xEnd Output vector for final state at time reached
     * @param t0 Initial time
     * @param t1 Target final time  
     * @param dxdt Derivative function
     * @return Time remaining (t1 - t_final_reached)
     */
    double ode(const std::vector<double>& x0, 
               std::vector<double>& xEnd, 
               double t0, 
               double t1, 
               DerivFunc dxdt) override;

    /**
     * @brief Set the integration step size
     * @param stepSize New step size (must be positive)
     */
    void setStepSize(double stepSize) override;

    /**
     * @brief Get the current step size
     * @return Current integration step size
     */
    double getStepSize() const override;
};

/**
 * @brief Factory function to create ODE solver instances
 * @param solverType Type of solver ("euler", "rk4", etc.)
 * @param stepSize Integration step size
 * @return Unique pointer to ODE solver instance
 */
std::unique_ptr<ODESolver> createODESolver(const std::string& solverType, double stepSize = 0.01);

} // namespace animation

#endif // ODE_SOLVER_HPP