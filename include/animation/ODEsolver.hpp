//==============================================================================
// ODEsolver.hpp - ODE Solver Interface for SauceEngine
//==============================================================================

#ifndef ODESOLVER_HPP
#define ODESOLVER_HPP

#include <vector>
#include <functional>
#include <memory>
#include <string>

namespace animation {

//==============================================================================
// Type Definitions
//==============================================================================

using DerivFunc = std::function<void(double t, 
                                     const std::vector<double>& x, 
                                     std::vector<double>& dxdt)>;

//==============================================================================
// ODESolver Interface
//==============================================================================

class ODESolver {
public:
    virtual ~ODESolver() = default;
    
    /**
     * Solve an ODE system from t0 to t1
     * @param x0 Initial state vector
     * @param xEnd Output: Final state vector
     * @param t0 Initial time
     * @param t1 Final time
     * @param dxdt Derivative function
     * @return Time remaining (should be close to 0)
     */
    virtual double ode(const std::vector<double>& x0, 
                      std::vector<double>& xEnd, 
                      double t0, 
                      double t1, 
                      DerivFunc dxdt) = 0;
    
    virtual void setStepSize(double stepSize) = 0;
    virtual double getStepSize() const = 0;
};

//==============================================================================
// EulerSolver - First-order Euler method
//==============================================================================

class EulerSolver : public ODESolver {
public:
    explicit EulerSolver(double stepSize = 0.01);
    
    double ode(const std::vector<double>& x0, 
              std::vector<double>& xEnd, 
              double t0, 
              double t1, 
              DerivFunc dxdt) override;
    
    void setStepSize(double stepSize) override;
    double getStepSize() const override;

private:
    double m_stepSize;
};

//==============================================================================
// RK4Solver - Fourth-order Runge-Kutta method (more accurate!)
//==============================================================================

class RK4Solver : public ODESolver {
public:
    explicit RK4Solver(double stepSize = 0.01);
    
    double ode(const std::vector<double>& x0, 
              std::vector<double>& xEnd, 
              double t0, 
              double t1, 
              DerivFunc dxdt) override;
    
    void setStepSize(double stepSize) override;
    double getStepSize() const override;

private:
    double m_stepSize;
};

//==============================================================================
// Factory Function
//==============================================================================

/**
 * Create an ODE solver by type name
 * @param solverType "euler" or "rk4"
 * @param stepSize Integration step size
 * @return Unique pointer to solver
 */
std::unique_ptr<ODESolver> createODESolver(const std::string& solverType, 
                                           double stepSize = 0.01);

} // namespace animation

#endif // ANIMATION_ODESOLVER_HPP