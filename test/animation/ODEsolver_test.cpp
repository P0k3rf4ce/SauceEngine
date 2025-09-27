#include <gtest/gtest.h>
    #include <vector>
    #include "animation/ODEsolver.hpp"
    #include <cmath>

    using namespace animation;

void exponentialGrowth(double t, const std::vector<double>& x, std::vector<double>& xdot) {
    const double k = 1.0;
    xdot[0] = k * x[0];
}

void harmonicOscillator(double t, const std::vector<double>& x, std::vector<double>& xdot) {
    const double omega_squared = 4.0;
    xdot[0] = x[1];
    xdot[1] = -omega_squared * x[0];
}

void linearSystem(double t, const std::vector<double>& x, std::vector<double>& xdot) {
    xdot[0] = -1.0 * x[0];
    xdot[1] = -2.0 * x[1];
}

void constantDerivative(double t, const std::vector<double>& x, std::vector<double>& xdot) {
    const double c = 5.0;
    xdot[0] = c;
}

TEST(ODESolverTests, ExponentialGrowthAccuracy) {
    /**
     * @brief Test exponential growth ODE accuracy
     * 
     * Solves dx/dt = x with x(0) = 1 from t=0 to t=1
     * Analytical solution: x(1) = e ≈ 2.718
     */
    
    EulerSolver solver(0.001);
    std::vector<double> x0 = {1.0}; // Initial condition: x(0) = 1
    std::vector<double> xEnd;
    
    double timeLeft = solver.ode(x0, xEnd, 0.0, 1.0, exponentialGrowth);
    
    double analytical = std::exp(1.0); // e^1 ≈ 2.718
    double numerical = xEnd[0];
    double relativeError = std::abs(numerical - analytical) / analytical * 100.0;
    
    EXPECT_EQ(xEnd.size(), 1);
    EXPECT_LT(relativeError, 1.0); // Should be within 1% error
    EXPECT_NEAR(numerical, analytical, 0.03);
    EXPECT_NEAR(timeLeft, 0.0, 1e-12); // Should reach exactly t=1.0
}

TEST(ODESolverTests, HarmonicOscillatorSystem) {
    /**
     * @brief Test harmonic oscillator multi-dimensional system
     * 
     * Solves d²x/dt² = -4x converted to first-order system
     * Initial conditions: x(0) = 1, v(0) = 0
     */
    
    EulerSolver solver(0.001);
    std::vector<double> x0 = {1.0, 0.0}; // x(0) = 1, v(0) = 0
    std::vector<double> xEnd;
    
    double quarter_period = M_PI / 4.0; // π/4 for ω = 2
    double timeLeft = solver.ode(x0, xEnd, 0.0, quarter_period, harmonicOscillator);
    
    // At t = π/4, analytical solution
    double analytical_x = std::cos(2.0 * quarter_period); // cos(π/2) = 0
    double analytical_v = -2.0 * std::sin(2.0 * quarter_period); // -2*sin(π/2) = -2
    
    EXPECT_EQ(xEnd.size(), 2);
    EXPECT_NEAR(xEnd[0], analytical_x, 0.1);
    EXPECT_NEAR(xEnd[1], analytical_v, 0.2);
}

TEST(ODESolverTests, LinearSystemEvolution) {
    /**
     * @brief Test linear system with multiple components
     * 
     * System: dx₁/dt = -x₁, dx₂/dt = -2x₂
     * Initial conditions: x₁(0) = 2, x₂(0) = 3
     */
    
    EulerSolver solver(0.01);
    std::vector<double> x0 = {2.0, 3.0}; // x₁(0) = 2, x₂(0) = 3
    std::vector<double> xEnd;
    
    double t_final = 1.0;
    solver.ode(x0, xEnd, 0.0, t_final, linearSystem);
    
    // Analytical solutions
    double analytical_x1 = 2.0 * std::exp(-t_final);  // 2*exp(-1)
    double analytical_x2 = 3.0 * std::exp(-2.0 * t_final); // 3*exp(-2)
    
    EXPECT_EQ(xEnd.size(), 2);
    
    double error_x1 = std::abs(xEnd[0] - analytical_x1) / analytical_x1 * 100.0;
    double error_x2 = std::abs(xEnd[1] - analytical_x2) / analytical_x2 * 100.0;
    
    EXPECT_LT(error_x1, 5.0); // x₁ error < 5%
    EXPECT_LT(error_x2, 5.0); // x₂ error < 5%
}

TEST(ODESolverTests, ConstantDerivativeExactness) {
    /**
     * @brief Test constant derivative (linear function)
     * 
     * System: dx/dt = 5, x(0) = 1
     * Analytical solution: x(t) = 1 + 5*t
     * Euler method should be exact for linear functions
     */
    
    EulerSolver solver(0.1);
    std::vector<double> x0 = {1.0}; // x(0) = 1
    std::vector<double> xEnd;
    
    double t_final = 2.0;
    solver.ode(x0, xEnd, 0.0, t_final, constantDerivative);
    
    // With step size 0.1, we take exactly 20 steps to reach t=2.0
    // Analytical solution at t=2.0: x(2.0) = 1 + 5*2.0 = 11
    double analytical = 1.0 + 5.0 * 2.0; // 1 + 5*2 = 11
    
    EXPECT_EQ(xEnd.size(), 1);
    EXPECT_NEAR(xEnd[0], analytical, 1e-12); // Should be exact
}

TEST(ODESolverTests, NegativeStepSizeError) {
    /**
     * @brief Test constructor error handling for negative step size
     */
    
    EXPECT_THROW(EulerSolver(-0.01), std::invalid_argument);
}

TEST(ODESolverTests, ZeroStepSizeError) {
    /**
     * @brief Test constructor error handling for zero step size
     */
    
    EXPECT_THROW(EulerSolver(0.0), std::invalid_argument);
}

TEST(ODESolverTests, EmptyInitialConditionsError) {
    /**
     * @brief Test error handling for empty initial conditions
     */
    
    EulerSolver solver(0.01);
    std::vector<double> empty_x0;
    std::vector<double> xEnd;
    
    EXPECT_THROW(solver.ode(empty_x0, xEnd, 0.0, 1.0, exponentialGrowth), std::invalid_argument);
}

TEST(ODESolverTests, InvalidTimeRangeError) {
    /**
     * @brief Test error handling for invalid time ranges
     */
    
    EulerSolver solver(0.01);
    std::vector<double> x0 = {1.0};
    std::vector<double> xEnd;
    
    // t1 < t0
    EXPECT_THROW(solver.ode(x0, xEnd, 1.0, 0.0, exponentialGrowth), std::invalid_argument);
    
    // t1 == t0  
    EXPECT_THROW(solver.ode(x0, xEnd, 1.0, 1.0, exponentialGrowth), std::invalid_argument);
}

TEST(ODESolverTests, StepSizeModification) {
    /**
     * @brief Test step size getter/setter functionality
     */
    
    EulerSolver solver(0.001);
    
    // Test initial step size
    EXPECT_DOUBLE_EQ(solver.getStepSize(), 0.001);
    
    // Test setting new step size
    solver.setStepSize(0.05);
    EXPECT_DOUBLE_EQ(solver.getStepSize(), 0.05);
    
    // Test invalid step size modifications
    EXPECT_THROW(solver.setStepSize(0.0), std::invalid_argument);
    EXPECT_THROW(solver.setStepSize(-0.01), std::invalid_argument);
}

TEST(ODESolverTests, LargeStepSizeBoundary) {
    /**
     * @brief Test step size larger than integration interval and verify leftover time
     */
    
    EulerSolver solver(1.0); // Large step size
    std::vector<double> x0 = {1.0};
    std::vector<double> xEnd;
    
    // Integrate over small interval (0.1 < 1.0 step size)
    // With new implementation, no steps will be taken since 0.0 + 1.0 > 0.1
    double timeLeft = solver.ode(x0, xEnd, 0.0, 0.1, constantDerivative);
    
    // Should return initial state since no steps were taken
    double expected = x0[0]; // Should remain 1.0
    EXPECT_NEAR(xEnd[0], expected, 1e-12);
    
    // Should return the full time interval as leftover
    EXPECT_NEAR(timeLeft, 0.1, 1e-12);
}

TEST(ODESolverTests, FactoryFunctionCreation) {
    /**
     * @brief Test factory function for creating solvers
     */
    
    auto solver = createODESolver("euler", 0.01);
    
    ASSERT_NE(solver, nullptr);
    EXPECT_DOUBLE_EQ(solver->getStepSize(), 0.01);
    
    // Test that it works
    std::vector<double> x0 = {1.0};
    std::vector<double> xEnd;
    
    EXPECT_NO_THROW(solver->ode(x0, xEnd, 0.0, 0.1, exponentialGrowth));
    EXPECT_EQ(xEnd.size(), 1);
}

TEST(ODESolverTests, FactoryDefaultStepSize) {
    /**
     * @brief Test factory function with default step size
     */
    
    auto solver = createODESolver("euler");
    
    ASSERT_NE(solver, nullptr);
    EXPECT_DOUBLE_EQ(solver->getStepSize(), 0.01);
}

TEST(ODESolverTests, FactoryUnknownSolverType) {
    /**
     * @brief Test factory function error handling
     */
    
    EXPECT_THROW(createODESolver("unknown"), std::invalid_argument);
    EXPECT_THROW(createODESolver("rk4"), std::invalid_argument);
    EXPECT_THROW(createODESolver(""), std::invalid_argument);
}

TEST(ODESolverTests, StepSizeAccuracyComparison) {
    /**
     * @brief Test accuracy improvement with smaller step sizes
     */
    
    EulerSolver solver(0.1);
    std::vector<double> x0 = {1.0};
    std::vector<double> result_coarse, result_fine;
    
    // Solve with coarse step size
    solver.ode(x0, result_coarse, 0.0, 1.0, exponentialGrowth);
    
    // Solve with fine step size
    solver.setStepSize(0.001);
    solver.ode(x0, result_fine, 0.0, 1.0, exponentialGrowth);
    
    double analytical = std::exp(1.0);
    double error_coarse = std::abs(result_coarse[0] - analytical) / analytical;
    double error_fine = std::abs(result_fine[0] - analytical) / analytical;
    
    EXPECT_LT(error_fine, error_coarse);
    EXPECT_LT(error_fine, 0.01);
}

TEST(ODESolverTests, MultiDimensionalSystemEvolution) {
    /**
     * @brief Test multi-dimensional system evolution
     */
    
    EulerSolver solver(0.001);
    std::vector<double> x0 = {1.0, 2.0, 3.0}; // 3D system
    std::vector<double> xEnd;
    
    solver.ode(x0, xEnd, 0.0, 1.0, linearSystem);
    
    EXPECT_EQ(xEnd.size(), 3);
    
    // For the linear system we defined: dx₁/dt = -x₁, dx₂/dt = -2x₂
    // Only first 2 components should decay, third should remain unchanged
    EXPECT_LT(std::abs(xEnd[0]), std::abs(x0[0])); // x₁ should decay
    EXPECT_LT(std::abs(xEnd[1]), std::abs(x0[1])); // x₂ should decay  
    EXPECT_GT(xEnd[0], 0.0);
    EXPECT_GT(xEnd[1], 0.0);
}

TEST(ODESolverTests, PolymorphicUsage) {
    /**
     * @brief Test polymorphic usage through base class pointer
     */
    
    std::unique_ptr<ODESolver> baseSolver = std::make_unique<EulerSolver>(0.01);
    
    std::vector<double> x0 = {1.0};
    std::vector<double> xEnd;
    
    double timeLeft = 0.0;
    EXPECT_NO_THROW(timeLeft = baseSolver->ode(x0, xEnd, 0.0, 0.5, exponentialGrowth));
    EXPECT_EQ(xEnd.size(), 1);
    
    // With step size 0.01, we take exactly 50 steps to reach t=0.5
    // Expected value should be close to e^0.5 ≈ 1.649
    double expected_approx = std::exp(0.5); // e^0.5 ≈ 1.649
    EXPECT_NEAR(xEnd[0], expected_approx, 0.01);
    EXPECT_NEAR(timeLeft, 0.0, 1e-12); // Should reach exactly t=0.5
}

TEST(ODESolverTests, LeftoverTimeCalculation) {
    /**
     * @brief Test that leftover time is correctly calculated when step size doesn't divide evenly
     */
    
    EulerSolver solver(0.03); // Step size that doesn't divide evenly into 0.1
    std::vector<double> x0 = {1.0};
    std::vector<double> xEnd;
    
    // Target time 0.1, step size 0.03
    // Steps taken: 0.0, 0.03, 0.06, 0.09
    // Next step would be 0.12 which overshoots 0.1
    // So we stop at t=0.09, leftover = 0.1 - 0.09 = 0.01
    double timeLeft = solver.ode(x0, xEnd, 0.0, 0.1, constantDerivative);
    
    EXPECT_EQ(xEnd.size(), 1);
    
    // Should have taken 3 steps: 0.0->0.03->0.06->0.09
    // At t=0.09: x = 1 + 5*0.09 = 1.45
    double expected_state = 1.0 + 5.0 * 0.09; // 1.45
    EXPECT_NEAR(xEnd[0], expected_state, 1e-12);
    
    // Leftover time should be 0.1 - 0.09 = 0.01
    double expected_leftover = 0.1 - 0.09; // 0.01
    EXPECT_NEAR(timeLeft, expected_leftover, 1e-12);
}

TEST(ODESolverTests, EdgeCaseVerySmallStepSize) {
    /**
     * @brief Test edge case with very small step size
     */
    
    EulerSolver solver(1e-6); // Very small step size
    std::vector<double> x0 = {2.0};
    std::vector<double> xEnd;
    
    double timeLeft = solver.ode(x0, xEnd, 0.0, 1e-3, constantDerivative);
    
    EXPECT_EQ(xEnd.size(), 1);
    EXPECT_NEAR(timeLeft, 0.0, 1e-12); // Should reach target exactly
    
    // Should be very close to analytical solution
    double expected = 2.0 + 5.0 * 1e-3;
    EXPECT_NEAR(xEnd[0], expected, 1e-10);
}

TEST(ODESolverTests, ZeroTimeInterval) {
    /**
     * @brief Test behavior with zero time interval
     */
    
    EulerSolver solver(0.01);
    std::vector<double> x0 = {3.0};
    std::vector<double> xEnd;
    
    // Should throw for zero time interval
    EXPECT_THROW(solver.ode(x0, xEnd, 1.0, 1.0, constantDerivative), std::invalid_argument);
}

TEST(ODESolverTests, BackwardTimeInterval) {
    /**
     * @brief Test behavior with backward time interval
     */
    
    EulerSolver solver(0.01);
    std::vector<double> x0 = {3.0};
    std::vector<double> xEnd;
    
    // Should throw for backward time interval
    EXPECT_THROW(solver.ode(x0, xEnd, 2.0, 1.0, constantDerivative), std::invalid_argument);
}

TEST(ODESolverTests, SingleStepIntegration) {
    /**
     * @brief Test integration that takes exactly one step
     */
    
    EulerSolver solver(0.5);
    std::vector<double> x0 = {1.0};
    std::vector<double> xEnd;
    
    // Integration from 0 to 0.5 with step size 0.5 should take exactly one step
    double timeLeft = solver.ode(x0, xEnd, 0.0, 0.5, constantDerivative);
    
    EXPECT_EQ(xEnd.size(), 1);
    EXPECT_NEAR(timeLeft, 0.0, 1e-12); // Should reach target exactly
    
    // After one step: x = 1 + 5*0.5 = 3.5
    double expected = 1.0 + 5.0 * 0.5;
    EXPECT_NEAR(xEnd[0], expected, 1e-12);
}

TEST(ODESolverTests, MultiDimensionalConstantDerivative) {
    /**
     * @brief Test multi-dimensional system with constant derivatives
     */
    
    auto multiConstantDerivative = [](double t, const std::vector<double>& x, std::vector<double>& xdot) {
        xdot[0] = 2.0; // dx1/dt = 2
        xdot[1] = -3.0; // dx2/dt = -3
        xdot[2] = 1.5; // dx3/dt = 1.5
    };
    
    EulerSolver solver(0.1);
    std::vector<double> x0 = {1.0, 2.0, 3.0};
    std::vector<double> xEnd;
    
    double timeLeft = solver.ode(x0, xEnd, 0.0, 1.0, multiConstantDerivative);
    
    EXPECT_EQ(xEnd.size(), 3);
    EXPECT_NEAR(timeLeft, 0.0, 1e-12);
    
    // After t=1.0: x1 = 1 + 2*1 = 3, x2 = 2 - 3*1 = -1, x3 = 3 + 1.5*1 = 4.5
    EXPECT_NEAR(xEnd[0], 3.0, 1e-12);
    EXPECT_NEAR(xEnd[1], -1.0, 1e-12);
    EXPECT_NEAR(xEnd[2], 4.5, 1e-12);
}

TEST(ODESolverTests, StepSizeModificationWithIntegration) {
    /**
     * @brief Test that step size changes affect subsequent integrations
     */
    
    EulerSolver solver(0.1);
    std::vector<double> x0 = {0.0};
    std::vector<double> xEnd1, xEnd2;
    
    // First integration with coarse step size
    double timeLeft1 = solver.ode(x0, xEnd1, 0.0, 0.5, constantDerivative);
    
    // Change step size and integrate again
    solver.setStepSize(0.05);
    double timeLeft2 = solver.ode(x0, xEnd2, 0.0, 0.5, constantDerivative);
    
    EXPECT_NEAR(timeLeft1, 0.0, 1e-12);
    EXPECT_NEAR(timeLeft2, 0.0, 1e-12);
    
    // Both should reach the same final value (since it's a linear function)
    double expected = 0.0 + 5.0 * 0.5; // 2.5
    EXPECT_NEAR(xEnd1[0], expected, 1e-12);
    EXPECT_NEAR(xEnd2[0], expected, 1e-12);
}

TEST(ODESolverTests, FactoryErrorHandlingEdgeCases) {
    /**
     * @brief Test additional factory function error cases
     */
    
    // Test with very small step size
    auto solver1 = createODESolver("euler", 1e-10);
    ASSERT_NE(solver1, nullptr);
    EXPECT_DOUBLE_EQ(solver1->getStepSize(), 1e-10);
    
    // Test with mixed case (should still fail)
    EXPECT_THROW(createODESolver("Euler"), std::invalid_argument);
    EXPECT_THROW(createODESolver("EULER"), std::invalid_argument);
}

TEST(ODESolverTests, NumericalStabilityWithLargeValues) {
    /**
     * @brief Test numerical stability with large initial values
     */
    
    auto exponentialDecay = [](double t, const std::vector<double>& x, std::vector<double>& xdot) {
        xdot[0] = -0.1 * x[0]; // Decay with rate 0.1
    };
    
    EulerSolver solver(0.01);
    std::vector<double> x0 = {1000.0}; // Large initial value
    std::vector<double> xEnd;
    
    double timeLeft = solver.ode(x0, xEnd, 0.0, 5.0, exponentialDecay);
    
    EXPECT_EQ(xEnd.size(), 1);
    EXPECT_NEAR(timeLeft, 0.0, 1e-12);
    
    // Should decay significantly: x(5) = 1000 * exp(-0.1 * 5) ≈ 606.5
    double expected = 1000.0 * std::exp(-0.5);
    EXPECT_NEAR(xEnd[0], expected, 10.0); // Allow some tolerance for Euler method
    EXPECT_LT(xEnd[0], x0[0]); // Should definitely decay
}

TEST(ODESolverTests, EmptyDerivativeVector) {
    /**
     * @brief Test behavior when derivative function doesn't resize xdot properly
     */
    
    auto badDerivative = [](double t, const std::vector<double>& x, std::vector<double>& xdot) {
        // Intentionally don't modify xdot - it should be pre-sized
        // This tests that our implementation properly sizes xdot
        if (xdot.size() >= 1) {
            xdot[0] = 1.0;
        }
    };
    
    EulerSolver solver(0.1);
    std::vector<double> x0 = {1.0};
    std::vector<double> xEnd;
    
    // Should work because our implementation pre-sizes xdot
    double timeLeft = solver.ode(x0, xEnd, 0.0, 0.1, badDerivative);
    
    EXPECT_EQ(xEnd.size(), 1);
    EXPECT_NEAR(xEnd[0], 1.1, 1e-12); // x = 1 + 1*0.1
}

TEST(ODESolverTests, FactoryNegativeStepSize) {
    /**
     * @brief Test factory function with invalid step size
     */
    
    // Should throw when step size is negative
    EXPECT_THROW(createODESolver("euler", -0.01), std::invalid_argument);
    
    // Should throw when step size is zero
    EXPECT_THROW(createODESolver("euler", 0.0), std::invalid_argument);
}

TEST(ODESolverTests, VariousStepSizeValues) {
    /**
     * @brief Test with various step size values to hit different code paths
     */
    
    // Test with step size that exactly divides time interval
    {
        EulerSolver solver(0.25); // Divides 1.0 exactly
        std::vector<double> x0 = {0.0};
        std::vector<double> xEnd;
        
        double timeLeft = solver.ode(x0, xEnd, 0.0, 1.0, constantDerivative);
        EXPECT_NEAR(timeLeft, 0.0, 1e-12);
        EXPECT_NEAR(xEnd[0], 5.0, 1e-12); // 0 + 5*1 = 5
    }
    
    // Test with step size larger than interval (different from LargeStepSizeBoundary)
    {
        EulerSolver solver(2.0); // Much larger than interval
        std::vector<double> x0 = {10.0};
        std::vector<double> xEnd;
        
        double timeLeft = solver.ode(x0, xEnd, 0.0, 0.5, constantDerivative);
        EXPECT_NEAR(timeLeft, 0.5, 1e-12); // Full interval left over
        EXPECT_NEAR(xEnd[0], 10.0, 1e-12); // No change
    }
}

TEST(ODESolverTests, PrecisionBoundaryTest) {
    /**
     * @brief Test floating-point precision boundary conditions
     */
    
    // Test with step size that causes precision issues
    EulerSolver solver(0.1);
    std::vector<double> x0 = {1.0};
    std::vector<double> xEnd;
    
    // Use a time that might cause floating-point precision issues
    double timeLeft = solver.ode(x0, xEnd, 0.0, 0.3, constantDerivative);
    
    // Should take exactly 3 steps and reach t=0.3
    EXPECT_NEAR(timeLeft, 0.0, 1e-12);
    EXPECT_NEAR(xEnd[0], 1.0 + 5.0 * 0.3, 1e-12); // 2.5
}

TEST(ODESolverTests, AlternativeDerivativeFunctions) {
    /**
     * @brief Test with different types of derivative functions to increase coverage
     */
    
    // Test with quadratic growth
    auto quadraticGrowth = [](double t, const std::vector<double>& x, std::vector<double>& xdot) {
        xdot[0] = t * t; // dx/dt = t^2
    };
    
    EulerSolver solver(0.1);
    std::vector<double> x0 = {0.0};
    std::vector<double> xEnd;
    
    double timeLeft = solver.ode(x0, xEnd, 0.0, 1.0, quadraticGrowth);
    
    EXPECT_EQ(xEnd.size(), 1);
    EXPECT_NEAR(timeLeft, 0.0, 1e-12);
    // Should be approximately integral of t^2 from 0 to 1 = 1/3 ≈ 0.333
    EXPECT_GT(xEnd[0], 0.25); // Should be positive and reasonable
    EXPECT_LT(xEnd[0], 0.5);
}

TEST(ODESolverTests, MixedSignDerivatives) {
    /**
     * @brief Test multi-dimensional system with mixed positive/negative derivatives
     */
    
    auto mixedDerivatives = [](double t, const std::vector<double>& x, std::vector<double>& xdot) {
        xdot[0] = 2.0 * x[0];   // Exponential growth
        xdot[1] = -0.5 * x[1];  // Exponential decay
        xdot[2] = 3.0;          // Constant growth
    };
    
    EulerSolver solver(0.01);
    std::vector<double> x0 = {1.0, 4.0, 0.0};
    std::vector<double> xEnd;
    
    double timeLeft = solver.ode(x0, xEnd, 0.0, 0.5, mixedDerivatives);
    
    EXPECT_EQ(xEnd.size(), 3);
    EXPECT_NEAR(timeLeft, 0.0, 1e-12);
    
    // First component should grow
    EXPECT_GT(xEnd[0], x0[0]);
    
    // Second component should decay
    EXPECT_LT(xEnd[1], x0[1]);
    
    // Third component should increase linearly: 0 + 3*0.5 = 1.5
    EXPECT_NEAR(xEnd[2], 1.5, 0.05);
}

TEST(ODESolverTests, ErrorConditionsInSetStepSize) {
    /**
     * @brief Test all error conditions in setStepSize to ensure branch coverage
     */
    
    EulerSolver solver(0.1);
    
    // Test setting valid step sizes
    EXPECT_NO_THROW(solver.setStepSize(0.001));
    EXPECT_NO_THROW(solver.setStepSize(1.0));
    EXPECT_NO_THROW(solver.setStepSize(100.0));
    
    // Test invalid step sizes
    EXPECT_THROW(solver.setStepSize(0.0), std::invalid_argument);
    EXPECT_THROW(solver.setStepSize(-1e-10), std::invalid_argument);
    EXPECT_THROW(solver.setStepSize(-1.0), std::invalid_argument);
}

TEST(ODESolverTests, ConstructorBoundaryValues) {
    /**
     * @brief Test constructor with boundary step size values
     */
    
    // Test very small positive step size
    EXPECT_NO_THROW(EulerSolver(1e-15));
    
    // Test large step size
    EXPECT_NO_THROW(EulerSolver(1e6));
    
    // Test exactly zero (should throw)
    EXPECT_THROW(EulerSolver(0.0), std::invalid_argument);
    
    // Test negative values close to zero
    EXPECT_THROW(EulerSolver(-1e-15), std::invalid_argument);
}