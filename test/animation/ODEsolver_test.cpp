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
    
    solver.ode(x0, xEnd, 0.0, 1.0, exponentialGrowth);
    
    double analytical = std::exp(1.0); // e^1 ≈ 2.718
    double numerical = xEnd[0];
    double relativeError = std::abs(numerical - analytical) / analytical * 100.0;
    
    EXPECT_EQ(xEnd.size(), 1);
    EXPECT_LT(relativeError, 1.0); // Should be within 1% error
    EXPECT_NEAR(numerical, analytical, 0.03);
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
    solver.ode(x0, xEnd, 0.0, quarter_period, harmonicOscillator);
    
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
    
    // Analytical solution: x(t) = 1 + 5*t
    double analytical = 1.0 + 5.0 * t_final; // 1 + 5*2 = 11
    
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
     * @brief Test step size larger than integration interval
     */
    
    EulerSolver solver(1.0); // Large step size
    std::vector<double> x0 = {1.0};
    std::vector<double> xEnd;
    
    // Integrate over small interval
    solver.ode(x0, xEnd, 0.0, 0.1, constantDerivative);
    
    // Should still work correctly by adjusting step size
    double expected = 1.0 + 5.0 * 0.1; // 1.5
    EXPECT_NEAR(xEnd[0], expected, 1e-12);
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
    
    EXPECT_NO_THROW(baseSolver->ode(x0, xEnd, 0.0, 0.5, exponentialGrowth));
    EXPECT_EQ(xEnd.size(), 1);
    
    double expected_approx = std::exp(0.5); // e^0.5 ≈ 1.649
    EXPECT_NEAR(xEnd[0], expected_approx, 0.01);
}