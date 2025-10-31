//==============================================================================
// ODESolverTest.cpp - Unit Tests and Examples for SauceEngine ODE Solver
//==============================================================================

#include "../../include/animation/ODESolver.h"  // Relative path
#include <iostream>
#include <cmath>
#include <cassert>

using namespace SauceEngine;

//==============================================================================
// Test Derivative Functions
//==============================================================================

/**
 * @brief Example derivative function for exponential growth: dx/dt = k*x
 * 
 * This represents the simple exponential growth ODE with analytical solution:
 * x(t) = x0 * exp(k * t)
 */
void exponentialGrowth(double t, const std::vector<double>& x, std::vector<double>& xdot) {
    const double k = 1.0; // Growth rate
    xdot[0] = k * x[0];
}

/**
 * @brief Example derivative function for harmonic oscillator: d²x/dt² = -ω²x
 * 
 * This is converted to a first-order system:
 * dx₁/dt = x₂ (velocity)
 * dx₂/dt = -ω²x₁ (acceleration)
 */
void harmonicOscillator(double t, const std::vector<double>& x, std::vector<double>& xdot) {
    const double omega_squared = 4.0; // ω² = 4, so ω = 2, period = π
    xdot[0] = x[1];                    // dx/dt = v
    xdot[1] = -omega_squared * x[0];   // dv/dt = -ω²x
}

/**
 * @brief Linear system test: dx/dt = A*x where A = [-1, 0; 0, -2]
 * 
 * Analytical solution: x₁(t) = x₁₀*exp(-t), x₂(t) = x₂₀*exp(-2t)
 */
void linearSystem(double t, const std::vector<double>& x, std::vector<double>& xdot) {
    xdot[0] = -1.0 * x[0];  // dx₁/dt = -x₁
    xdot[1] = -2.0 * x[1];  // dx₂/dt = -2x₂
}

/**
 * @brief Constant derivative test: dx/dt = c
 * 
 * Analytical solution: x(t) = x₀ + c*t
 */
void constantDerivative(double t, const std::vector<double>& x, std::vector<double>& xdot) {
    const double c = 5.0;
    xdot[0] = c;
}

//==============================================================================
// Test Functions
//==============================================================================

/**
 * @brief Test exponential growth ODE
 */
void testExponentialGrowth() {
    std::cout << "Test 1: Exponential Growth (dx/dt = x)\n";
    EulerSolver solver(0.001); // Small step size for accuracy
    
    std::vector<double> x0 = {1.0}; // Initial condition: x(0) = 1
    std::vector<double> xEnd;
    
    solver.ode(x0, xEnd, 0.0, 1.0, exponentialGrowth);
    
    double analytical = std::exp(1.0); // e^1 ≈ 2.718
    double numerical = xEnd[0];
    double error = std::abs(numerical - analytical) / analytical * 100.0;
    
    std::cout << "  Analytical solution: " << analytical << std::endl;
    std::cout << "  Numerical solution:  " << numerical << std::endl;
    std::cout << "  Relative error:      " << error << "%\n\n";
    
    assert(error < 1.0); // Should be within 1% for this step size
}

/**
 * @brief Test harmonic oscillator system
 */
void testHarmonicOscillator() {
    std::cout << "Test 2: Harmonic Oscillator (d²x/dt² = -4x)\n";
    EulerSolver solver(0.001);
    
    std::vector<double> x0 = {1.0, 0.0}; // x(0) = 1, v(0) = 0
    std::vector<double> xEnd;
    
    double quarter_period = M_PI / 4.0; // π/4 for ω = 2
    solver.ode(x0, xEnd, 0.0, quarter_period, harmonicOscillator);
    
    // At t = π/4, analytical solution should be approximately x ≈ 0.707, v ≈ -1.414
    double analytical_x = std::cos(2.0 * quarter_period); // cos(π/2) = 0
    double analytical_v = -2.0 * std::sin(2.0 * quarter_period); // -2*sin(π/2) = -2
    
    std::cout << "  Analytical: x = " << analytical_x << ", v = " << analytical_v << std::endl;
    std::cout << "  Numerical:  x = " << xEnd[0] << ", v = " << xEnd[1] << std::endl;
    
    double error_x = std::abs(xEnd[0] - analytical_x);
    double error_v = std::abs(xEnd[1] - analytical_v);
    
    std::cout << "  Position error: " << error_x << std::endl;
    std::cout << "  Velocity error: " << error_v << "\n\n";
    
    assert(error_x < 0.1 && error_v < 0.2); // Reasonable tolerance for Euler method
}

/**
 * @brief Test linear system with multiple components
 */
void testLinearSystem() {
    std::cout << "Test 3: Linear System (dx₁/dt = -x₁, dx₂/dt = -2x₂)\n";
    EulerSolver solver(0.01);
    
    std::vector<double> x0 = {2.0, 3.0}; // x₁(0) = 2, x₂(0) = 3
    std::vector<double> xEnd;
    
    double t_final = 1.0;
    solver.ode(x0, xEnd, 0.0, t_final, linearSystem);
    
    // Analytical solutions
    double analytical_x1 = 2.0 * std::exp(-t_final);  // 2*exp(-1)
    double analytical_x2 = 3.0 * std::exp(-2.0 * t_final); // 3*exp(-2)
    
    std::cout << "  Analytical: x₁ = " << analytical_x1 << ", x₂ = " << analytical_x2 << std::endl;
    std::cout << "  Numerical:  x₁ = " << xEnd[0] << ", x₂ = " << xEnd[1] << std::endl;
    
    double error_x1 = std::abs(xEnd[0] - analytical_x1) / analytical_x1 * 100.0;
    double error_x2 = std::abs(xEnd[1] - analytical_x2) / analytical_x2 * 100.0;
    
    std::cout << "  Relative error x₁: " << error_x1 << "%" << std::endl;
    std::cout << "  Relative error x₂: " << error_x2 << "%\n\n";
    
    assert(error_x1 < 5.0 && error_x2 < 5.0); // Within 5% for this step size
}

/**
 * @brief Test constant derivative (linear function)
 */
void testConstantDerivative() {
    std::cout << "Test 4: Constant Derivative (dx/dt = 5)\n";
    EulerSolver solver(0.1);
    
    std::vector<double> x0 = {1.0}; // x(0) = 1
    std::vector<double> xEnd;
    
    double t_final = 2.0;
    solver.ode(x0, xEnd, 0.0, t_final, constantDerivative);
    
    // Analytical solution: x(t) = 1 + 5*t
    double analytical = 1.0 + 5.0 * t_final; // 1 + 5*2 = 11
    double numerical = xEnd[0];
    
    std::cout << "  Analytical solution: " << analytical << std::endl;
    std::cout << "  Numerical solution:  " << numerical << std::endl;
    std::cout << "  Absolute error:      " << std::abs(numerical - analytical) << "\n\n";
    
    // For linear functions, Euler method should be exact (within floating point precision)
    assert(std::abs(numerical - analytical) < 1e-12);
}

/**
 * @brief Test error handling and edge cases
 */
void testErrorHandling() {
    std::cout << "Test 5: Error Handling\n";
    EulerSolver solver(0.01);
    
    std::vector<double> x0 = {1.0};
    std::vector<double> xEnd;
    
    // Test invalid step size
    try {
        EulerSolver invalidSolver(-0.01);
        assert(false); // Should not reach here
    } catch (const std::invalid_argument&) {
        std::cout << "  ✓ Correctly caught negative step size error\n";
    }
    
    // Test empty initial conditions
    try {
        std::vector<double> empty_x0;
        solver.ode(empty_x0, xEnd, 0.0, 1.0, exponentialGrowth);
        assert(false); // Should not reach here
    } catch (const std::invalid_argument&) {
        std::cout << "  ✓ Correctly caught empty initial conditions error\n";
    }
    
    // Test invalid time range
    try {
        solver.ode(x0, xEnd, 1.0, 0.0, exponentialGrowth); // t1 < t0
        assert(false); // Should not reach here
    } catch (const std::invalid_argument&) {
        std::cout << "  ✓ Correctly caught invalid time range error\n";
    }
    
    // Test equal time range
    try {
        solver.ode(x0, xEnd, 1.0, 1.0, exponentialGrowth); // t1 == t0
        assert(false); // Should not reach here
    } catch (const std::invalid_argument&) {
        std::cout << "  ✓ Correctly caught equal time range error\n";
    }
    
    std::cout << std::endl;
}

/**
 * @brief Test step size modification
 */
void testStepSizeModification() {
    std::cout << "Test 6: Step Size Modification\n";
    EulerSolver solver(0.1);
    
    // Test getter
    assert(solver.getStepSize() == 0.1);
    std::cout << "  ✓ Initial step size correctly retrieved: " << solver.getStepSize() << std::endl;
    
    // Test setter
    solver.setStepSize(0.05);
    assert(solver.getStepSize() == 0.05);
    std::cout << "  ✓ Step size correctly modified: " << solver.getStepSize() << std::endl;
    
    // Test invalid step size modification
    try {
        solver.setStepSize(0.0);
        assert(false); // Should not reach here
    } catch (const std::invalid_argument&) {
        std::cout << "  ✓ Correctly caught zero step size error\n";
    }
    
    try {
        solver.setStepSize(-0.01);
        assert(false); // Should not reach here
    } catch (const std::invalid_argument&) {
        std::cout << "  ✓ Correctly caught negative step size error\n";
    }
    
    std::cout << std::endl;
}

/**
 * @brief Test factory function
 */
void testFactoryFunction() {
    std::cout << "Test 7: Factory Function\n";
    auto solver = createODESolver("euler", 0.01);
    
    std::vector<double> x0 = {1.0};
    std::vector<double> xEnd;
    
    solver->ode(x0, xEnd, 0.0, 0.1, exponentialGrowth);
    std::cout << "  ✓ Factory-created solver works correctly\n";
    
    // Test step size
    assert(solver->getStepSize() == 0.01);
    std::cout << "  ✓ Factory-created solver has correct step size\n";
    
    // Test unknown solver type
    try {
        auto unknown = createODESolver("unknown");
        assert(false); // Should not reach here
    } catch (const std::invalid_argument&) {
        std::cout << "  ✓ Correctly caught unknown solver type error\n";
    }
    
    std::cout << std::endl;
}

/**
 * @brief Test step size boundary conditions
 */
void testStepSizeBoundary() {
    std::cout << "Test 8: Step Size Boundary Conditions\n";
    
    // Test with step size larger than integration interval
    EulerSolver solver(1.0); // Large step size
    
    std::vector<double> x0 = {1.0};
    std::vector<double> xEnd;
    
    // Integrate over small interval
    solver.ode(x0, xEnd, 0.0, 0.1, constantDerivative);
    
    // Should still work correctly by adjusting step size
    double expected = 1.0 + 5.0 * 0.1; // 1.5
    std::cout << "  Expected: " << expected << ", Got: " << xEnd[0] << std::endl;
    assert(std::abs(xEnd[0] - expected) < 1e-12);
    std::cout << "  ✓ Large step size correctly handled\n\n";
}

//==============================================================================
// Main Test Runner
//==============================================================================

/**
 * @brief Run all ODE solver tests
 */
void runAllTests() {
    std::cout << "=============================================================\n";
    std::cout << "      SauceEngine ODE Solver Test Suite\n";
    std::cout << "=============================================================\n\n";

    try {
        testExponentialGrowth();
        testHarmonicOscillator();
        testLinearSystem();
        testConstantDerivative();
        testErrorHandling();
        testStepSizeModification();
        testFactoryFunction();
        testStepSizeBoundary();
        
        std::cout << "=============================================================\n";
        std::cout << "            ALL TESTS PASSED! ✓\n";
        std::cout << "=============================================================\n";
        
    } catch (const std::exception& e) {
        std::cout << "=============================================================\n";
        std::cout << "            TEST FAILED! ✗\n";
        std::cout << "Error: " << e.what() << std::endl;
        std::cout << "=============================================================\n";
        throw;
    }
}

//==============================================================================
// Usage Examples
//==============================================================================

/**
 * @brief Demonstrate basic usage of the ODE solver
 */
void demonstrateUsage() {
    std::cout << "\n=============================================================\n";
    std::cout << "      SauceEngine ODE Solver Usage Examples\n";
    std::cout << "=============================================================\n\n";

    // Example 1: Simple exponential growth
    std::cout << "Example 1: Solving dx/dt = x with x(0) = 1\n";
    {
        EulerSolver solver(0.01);
        std::vector<double> x0 = {1.0};
        std::vector<double> xEnd;
        
        solver.ode(x0, xEnd, 0.0, 2.0, exponentialGrowth);
        std::cout << "  Solution at t=2: x = " << xEnd[0] 
                  << " (analytical: " << std::exp(2.0) << ")\n\n";
    }

    // Example 2: Using factory function
    std::cout << "Example 2: Using factory function\n";
    {
        auto solver = createODESolver("euler", 0.005);
        std::vector<double> x0 = {0.0, 1.0}; // Start at (0,1) for harmonic oscillator
        std::vector<double> xEnd;
        
        solver->ode(x0, xEnd, 0.0, M_PI/2.0, harmonicOscillator); // Quarter period
        std::cout << "  Harmonic oscillator at t=π/2: x = " << xEnd[0] 
                  << ", v = " << xEnd[1] << std::endl;
        std::cout << "  (Should be approximately x ≈ -1, v ≈ 0)\n\n";
    }

    // Example 3: Changing step size dynamically
    std::cout << "Example 3: Dynamic step size adjustment\n";
    {
        EulerSolver solver(0.1);
        std::vector<double> x0 = {1.0};
        std::vector<double> result1, result2;
        
        // Solve with coarse step size
        solver.ode(x0, result1, 0.0, 1.0, exponentialGrowth);
        
        // Solve with fine step size
        solver.setStepSize(0.001);
        solver.ode(x0, result2, 0.0, 1.0, exponentialGrowth);
        
        std::cout << "  Coarse step (h=0.1): x = " << result1[0] << std::endl;
        std::cout << "  Fine step (h=0.001): x = " << result2[0] << std::endl;
        std::cout << "  Analytical solution:  x = " << std::exp(1.0) << std::endl;
        std::cout << "  Fine step is more accurate!\n\n";
    }
}

//==============================================================================
// Entry Point
//==============================================================================

int main() {
    try {
        runAllTests();
        demonstrateUsage();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test suite failed with exception: " << e.what() << std::endl;
        return 1;
    }
}