#include <gtest/gtest.h>
#include <vector>
#include "animation/ODEsolver.hpp"
#include <cmath>
#ifdef _WIN32
    #include <corecrt_math_defines.h>
#endif

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

void exponentialDecay(double t, const std::vector<double>& x, std::vector<double>& xdot) {
    xdot[0] = -x[0];
}

void pendulum(double t, const std::vector<double>& x, std::vector<double>& xdot) {
    xdot[0] = x[1];              // dθ/dt = ω
    xdot[1] = -std::sin(x[0]);   // dω/dt = -sin(θ)
}

//==============================================================================
// Existing Euler Tests
//==============================================================================

TEST(ODESolverTests, ExponentialGrowthAccuracy) {
    EulerSolver solver(0.001);
    std::vector<double> x0 = {1.0};
    std::vector<double> xEnd;
    
    double timeLeft = solver.ode(x0, xEnd, 0.0, 1.0, exponentialGrowth);
    
    double analytical = std::exp(1.0);
    double numerical = xEnd[0];
    double relativeError = std::abs(numerical - analytical) / analytical * 100.0;
    
    EXPECT_EQ(xEnd.size(), 1);
    EXPECT_LT(relativeError, 1.0);
    EXPECT_NEAR(numerical, analytical, 0.03);
    EXPECT_NEAR(timeLeft, 0.0, 1e-12);
}

TEST(ODESolverTests, HarmonicOscillatorSystem) {
    EulerSolver solver(0.001);
    std::vector<double> x0 = {1.0, 0.0};
    std::vector<double> xEnd;
    
    double quarter_period = M_PI / 4.0;
    double timeLeft = solver.ode(x0, xEnd, 0.0, quarter_period, harmonicOscillator);
    
    double analytical_x = std::cos(2.0 * quarter_period);
    double analytical_v = -2.0 * std::sin(2.0 * quarter_period);
    
    EXPECT_EQ(xEnd.size(), 2);
    EXPECT_NEAR(xEnd[0], analytical_x, 0.1);
    EXPECT_NEAR(xEnd[1], analytical_v, 0.2);
}

TEST(ODESolverTests, LinearSystemEvolution) {
    EulerSolver solver(0.01);
    std::vector<double> x0 = {2.0, 3.0};
    std::vector<double> xEnd;
    
    double t_final = 1.0;
    solver.ode(x0, xEnd, 0.0, t_final, linearSystem);
    
    double analytical_x1 = 2.0 * std::exp(-t_final);
    double analytical_x2 = 3.0 * std::exp(-2.0 * t_final);
    
    EXPECT_EQ(xEnd.size(), 2);
    
    double error_x1 = std::abs(xEnd[0] - analytical_x1) / analytical_x1 * 100.0;
    double error_x2 = std::abs(xEnd[1] - analytical_x2) / analytical_x2 * 100.0;
    
    EXPECT_LT(error_x1, 5.0);
    EXPECT_LT(error_x2, 5.0);
}

TEST(ODESolverTests, ConstantDerivativeExactness) {
    EulerSolver solver(0.1);
    std::vector<double> x0 = {1.0};
    std::vector<double> xEnd;
    
    double t_final = 2.0;
    solver.ode(x0, xEnd, 0.0, t_final, constantDerivative);
    
    double analytical = 1.0 + 5.0 * 2.0;
    
    EXPECT_EQ(xEnd.size(), 1);
    EXPECT_NEAR(xEnd[0], analytical, 1e-12);
}

//==============================================================================
// RK4 Accuracy Tests - Show RK4 is much better than Euler
//==============================================================================

TEST(RK4SolverTests, ExponentialDecayAccuracy) {
    std::vector<double> x0 = {1.0};
    std::vector<double> xEnd_euler, xEnd_rk4;
    
    EulerSolver euler(0.01);
    RK4Solver rk4(0.01);
    
    euler.ode(x0, xEnd_euler, 0.0, 1.0, exponentialDecay);
    rk4.ode(x0, xEnd_rk4, 0.0, 1.0, exponentialDecay);
    
    double analytical = std::exp(-1.0);
    double error_euler = std::abs(xEnd_euler[0] - analytical);
    double error_rk4 = std::abs(xEnd_rk4[0] - analytical);
    
    // RK4 should be MUCH more accurate than Euler
    EXPECT_LT(error_rk4, error_euler);
    EXPECT_LT(error_rk4, 1e-6);  // RK4 should be extremely accurate
    EXPECT_GT(error_euler / error_rk4, 100.0);  // At least 100x better
}

TEST(RK4SolverTests, HarmonicOscillatorEnergyConservation) {
    std::vector<double> x0 = {1.0, 0.0};
    std::vector<double> xEnd_euler, xEnd_rk4;
    
    EulerSolver euler(0.01);
    RK4Solver rk4(0.01);
    
    // Integrate for one full period
    euler.ode(x0, xEnd_euler, 0.0, 2.0 * M_PI, harmonicOscillator);
    rk4.ode(x0, xEnd_rk4, 0.0, 2.0 * M_PI, harmonicOscillator);
    
    // Check return to initial position
    double pos_error_euler = std::abs(xEnd_euler[0] - 1.0);
    double pos_error_rk4 = std::abs(xEnd_rk4[0] - 1.0);
    
    // RK4 should conserve energy much better
    EXPECT_LT(pos_error_rk4, 1e-4);
    EXPECT_LT(pos_error_rk4, pos_error_euler);
}

TEST(RK4SolverTests, StepSizeComparison) {
    std::vector<double> x0 = {1.0};
    double analytical = std::exp(-1.0);
    
    std::vector<double> step_sizes = {0.1, 0.05, 0.01};
    
    for (double h : step_sizes) {
        std::vector<double> xEnd_euler, xEnd_rk4;
        
        EulerSolver euler(h);
        RK4Solver rk4(h);
        
        euler.ode(x0, xEnd_euler, 0.0, 1.0, exponentialDecay);
        rk4.ode(x0, xEnd_rk4, 0.0, 1.0, exponentialDecay);
        
        double error_euler = std::abs(xEnd_euler[0] - analytical);
        double error_rk4 = std::abs(xEnd_rk4[0] - analytical);
        
        // RK4 should always be more accurate
        EXPECT_LT(error_rk4, error_euler);
    }
}

TEST(RK4SolverTests, PendulumEnergyConservation) {
    std::vector<double> x0 = {0.1, 0.0};  // Small angle, starts at rest
    std::vector<double> xEnd;
    
    RK4Solver solver(0.001);
    solver.ode(x0, xEnd, 0.0, 10.0, pendulum);
    
    // Energy should be approximately conserved
    double E0 = 0.5 * x0[1] * x0[1] - std::cos(x0[0]);
    double E1 = 0.5 * xEnd[1] * xEnd[1] - std::cos(xEnd[0]);
    double energy_error = std::abs(E1 - E0);
    
    EXPECT_LT(energy_error, 0.01);
}

TEST(RK4SolverTests, FactoryCreation) {
    auto solver = createODESolver("rk4", 0.01);
    
    ASSERT_NE(solver, nullptr);
    EXPECT_DOUBLE_EQ(solver->getStepSize(), 0.01);
    
    std::vector<double> x0 = {1.0};
    std::vector<double> xEnd;
    
    EXPECT_NO_THROW(solver->ode(x0, xEnd, 0.0, 0.1, exponentialDecay));
    EXPECT_EQ(xEnd.size(), 1);
}

TEST(RK4SolverTests, StepSizeModification) {
    RK4Solver solver(0.001);
    
    EXPECT_DOUBLE_EQ(solver.getStepSize(), 0.001);
    
    solver.setStepSize(0.05);
    EXPECT_DOUBLE_EQ(solver.getStepSize(), 0.05);
    
    EXPECT_THROW(solver.setStepSize(0.0), std::invalid_argument);
    EXPECT_THROW(solver.setStepSize(-0.01), std::invalid_argument);
}

TEST(RK4SolverTests, ConstructorValidation) {
    EXPECT_NO_THROW(RK4Solver(0.01));
    EXPECT_NO_THROW(RK4Solver(1e-10));
    EXPECT_NO_THROW(RK4Solver(1.0));
    
    EXPECT_THROW(RK4Solver(0.0), std::invalid_argument);
    EXPECT_THROW(RK4Solver(-0.01), std::invalid_argument);
}

TEST(RK4SolverTests, MultiDimensionalSystem) {
    auto system = [](double t, const std::vector<double>& x, std::vector<double>& xdot) {
        xdot[0] = -x[0];
        xdot[1] = -2.0 * x[1];
        xdot[2] = -0.5 * x[2];
    };
    
    RK4Solver solver(0.01);
    std::vector<double> x0 = {1.0, 2.0, 3.0};
    std::vector<double> xEnd;
    
    solver.ode(x0, xEnd, 0.0, 1.0, system);
    
    EXPECT_EQ(xEnd.size(), 3);
    
    // All components should decay
    EXPECT_LT(xEnd[0], x0[0]);
    EXPECT_LT(xEnd[1], x0[1]);
    EXPECT_LT(xEnd[2], x0[2]);
    
    // Check approximate analytical solutions
    EXPECT_NEAR(xEnd[0], std::exp(-1.0), 0.001);
    EXPECT_NEAR(xEnd[1], 2.0 * std::exp(-2.0), 0.001);
    EXPECT_NEAR(xEnd[2], 3.0 * std::exp(-0.5), 0.001);
}

TEST(RK4SolverTests, ErrorHandling) {
    RK4Solver solver(0.01);
    std::vector<double> x0 = {1.0};
    std::vector<double> xEnd;
    
    // Invalid time range
    EXPECT_THROW(solver.ode(x0, xEnd, 1.0, 0.0, exponentialDecay), std::invalid_argument);
    EXPECT_THROW(solver.ode(x0, xEnd, 1.0, 1.0, exponentialDecay), std::invalid_argument);
    
    // Empty state vector
    std::vector<double> x_empty;
    EXPECT_THROW(solver.ode(x_empty, xEnd, 0.0, 1.0, exponentialDecay), std::invalid_argument);
}

//==============================================================================
// Remaining Original Tests
//==============================================================================

TEST(ODESolverTests, NegativeStepSizeError) {
    EXPECT_THROW(EulerSolver(-0.01), std::invalid_argument);
}

TEST(ODESolverTests, ZeroStepSizeError) {
    EXPECT_THROW(EulerSolver(0.0), std::invalid_argument);
}

TEST(ODESolverTests, EmptyInitialConditionsError) {
    EulerSolver solver(0.01);
    std::vector<double> empty_x0;
    std::vector<double> xEnd;
    
    EXPECT_THROW(solver.ode(empty_x0, xEnd, 0.0, 1.0, exponentialGrowth), std::invalid_argument);
}

TEST(ODESolverTests, InvalidTimeRangeError) {
    EulerSolver solver(0.01);
    std::vector<double> x0 = {1.0};
    std::vector<double> xEnd;
    
    EXPECT_THROW(solver.ode(x0, xEnd, 1.0, 0.0, exponentialGrowth), std::invalid_argument);
    EXPECT_THROW(solver.ode(x0, xEnd, 1.0, 1.0, exponentialGrowth), std::invalid_argument);
}

TEST(ODESolverTests, StepSizeModification) {
    EulerSolver solver(0.001);
    
    EXPECT_DOUBLE_EQ(solver.getStepSize(), 0.001);
    
    solver.setStepSize(0.05);
    EXPECT_DOUBLE_EQ(solver.getStepSize(), 0.05);
    
    EXPECT_THROW(solver.setStepSize(0.0), std::invalid_argument);
    EXPECT_THROW(solver.setStepSize(-0.01), std::invalid_argument);
}

TEST(ODESolverTests, LargeStepSizeBoundary) {
    EulerSolver solver(1.0);
    std::vector<double> x0 = {1.0};
    std::vector<double> xEnd;
    
    double timeLeft = solver.ode(x0, xEnd, 0.0, 0.1, constantDerivative);
    
    double expected = x0[0];
    EXPECT_NEAR(xEnd[0], expected, 1e-12);
    EXPECT_NEAR(timeLeft, 0.1, 1e-12);
}

TEST(ODESolverTests, FactoryFunctionCreation) {
    auto solver = createODESolver("euler", 0.01);
    
    ASSERT_NE(solver, nullptr);
    EXPECT_DOUBLE_EQ(solver->getStepSize(), 0.01);
    
    std::vector<double> x0 = {1.0};
    std::vector<double> xEnd;
    
    EXPECT_NO_THROW(solver->ode(x0, xEnd, 0.0, 0.1, exponentialGrowth));
    EXPECT_EQ(xEnd.size(), 1);
}

TEST(ODESolverTests, FactoryDefaultStepSize) {
    auto solver = createODESolver("euler");
    
    ASSERT_NE(solver, nullptr);
    EXPECT_DOUBLE_EQ(solver->getStepSize(), 0.01);
}

TEST(ODESolverTests, FactoryUnknownSolverType) {
    EXPECT_THROW(createODESolver("unknown"), std::invalid_argument);
    EXPECT_THROW(createODESolver(""), std::invalid_argument);
}

TEST(ODESolverTests, FactoryBothSolverTypes) {
    auto euler = createODESolver("euler", 0.01);
    auto rk4 = createODESolver("rk4", 0.01);
    
    ASSERT_NE(euler, nullptr);
    ASSERT_NE(rk4, nullptr);
}

TEST(ODESolverTests, StepSizeAccuracyComparison) {
    EulerSolver solver(0.1);
    std::vector<double> x0 = {1.0};
    std::vector<double> result_coarse, result_fine;
    
    solver.ode(x0, result_coarse, 0.0, 1.0, exponentialGrowth);
    
    solver.setStepSize(0.001);
    solver.ode(x0, result_fine, 0.0, 1.0, exponentialGrowth);
    
    double analytical = std::exp(1.0);
    double error_coarse = std::abs(result_coarse[0] - analytical) / analytical;
    double error_fine = std::abs(result_fine[0] - analytical) / analytical;
    
    EXPECT_LT(error_fine, error_coarse);
    EXPECT_LT(error_fine, 0.01);
}

TEST(ODESolverTests, MultiDimensionalSystemEvolution) {
    EulerSolver solver(0.001);
    std::vector<double> x0 = {1.0, 2.0, 3.0};
    std::vector<double> xEnd;
    
    solver.ode(x0, xEnd, 0.0, 1.0, linearSystem);
    
    EXPECT_EQ(xEnd.size(), 3);
    EXPECT_LT(std::abs(xEnd[0]), std::abs(x0[0]));
    EXPECT_LT(std::abs(xEnd[1]), std::abs(x0[1]));
    EXPECT_GT(xEnd[0], 0.0);
    EXPECT_GT(xEnd[1], 0.0);
}

TEST(ODESolverTests, PolymorphicUsage) {
    std::unique_ptr<ODESolver> baseSolver = std::make_unique<EulerSolver>(0.01);
    
    std::vector<double> x0 = {1.0};
    std::vector<double> xEnd;
    
    double timeLeft = 0.0;
    EXPECT_NO_THROW(timeLeft = baseSolver->ode(x0, xEnd, 0.0, 0.5, exponentialGrowth));
    EXPECT_EQ(xEnd.size(), 1);
    
    double expected_approx = std::exp(0.5);
    EXPECT_NEAR(xEnd[0], expected_approx, 0.01);
    EXPECT_NEAR(timeLeft, 0.0, 1e-12);
}

TEST(ODESolverTests, LeftoverTimeCalculation) {
    EulerSolver solver(0.03);
    std::vector<double> x0 = {1.0};
    std::vector<double> xEnd;
    
    double timeLeft = solver.ode(x0, xEnd, 0.0, 0.1, constantDerivative);
    
    EXPECT_EQ(xEnd.size(), 1);
    
    double expected_state = 1.0 + 5.0 * 0.09;
    EXPECT_NEAR(xEnd[0], expected_state, 1e-12);
    
    double expected_leftover = 0.1 - 0.09;
    EXPECT_NEAR(timeLeft, expected_leftover, 1e-12);
}

TEST(ODESolverTests, EdgeCaseVerySmallStepSize) {
    EulerSolver solver(1e-6);
    std::vector<double> x0 = {2.0};
    std::vector<double> xEnd;
    
    double timeLeft = solver.ode(x0, xEnd, 0.0, 1e-3, constantDerivative);
    
    EXPECT_EQ(xEnd.size(), 1);
    EXPECT_NEAR(timeLeft, 0.0, 1e-12);
    
    double expected = 2.0 + 5.0 * 1e-3;
    EXPECT_NEAR(xEnd[0], expected, 1e-10);
}

TEST(ODESolverTests, ZeroTimeInterval) {
    EulerSolver solver(0.01);
    std::vector<double> x0 = {3.0};
    std::vector<double> xEnd;
    
    EXPECT_THROW(solver.ode(x0, xEnd, 1.0, 1.0, constantDerivative), std::invalid_argument);
}

TEST(ODESolverTests, BackwardTimeInterval) {
    EulerSolver solver(0.01);
    std::vector<double> x0 = {3.0};
    std::vector<double> xEnd;
    
    EXPECT_THROW(solver.ode(x0, xEnd, 2.0, 1.0, constantDerivative), std::invalid_argument);
}

TEST(ODESolverTests, SingleStepIntegration) {
    EulerSolver solver(0.5);
    std::vector<double> x0 = {1.0};
    std::vector<double> xEnd;
    
    double timeLeft = solver.ode(x0, xEnd, 0.0, 0.5, constantDerivative);
    
    EXPECT_EQ(xEnd.size(), 1);
    EXPECT_NEAR(timeLeft, 0.0, 1e-12);
    
    double expected = 1.0 + 5.0 * 0.5;
    EXPECT_NEAR(xEnd[0], expected, 1e-12);
}

TEST(ODESolverTests, MultiDimensionalConstantDerivative) {
    auto multiConstantDerivative = [](double t, const std::vector<double>& x, std::vector<double>& xdot) {
        xdot[0] = 2.0;
        xdot[1] = -3.0;
        xdot[2] = 1.5;
    };
    
    EulerSolver solver(0.1);
    std::vector<double> x0 = {1.0, 2.0, 3.0};
    std::vector<double> xEnd;
    
    double timeLeft = solver.ode(x0, xEnd, 0.0, 1.0, multiConstantDerivative);
    
    EXPECT_EQ(xEnd.size(), 3);
    EXPECT_NEAR(timeLeft, 0.0, 1e-12);
    
    EXPECT_NEAR(xEnd[0], 3.0, 1e-12);
    EXPECT_NEAR(xEnd[1], -1.0, 1e-12);
    EXPECT_NEAR(xEnd[2], 4.5, 1e-12);
}

TEST(ODESolverTests, StepSizeModificationWithIntegration) {
    EulerSolver solver(0.1);
    std::vector<double> x0 = {0.0};
    std::vector<double> xEnd1, xEnd2;
    
    double timeLeft1 = solver.ode(x0, xEnd1, 0.0, 0.5, constantDerivative);
    
    solver.setStepSize(0.05);
    double timeLeft2 = solver.ode(x0, xEnd2, 0.0, 0.5, constantDerivative);
    
    EXPECT_NEAR(timeLeft1, 0.0, 1e-12);
    EXPECT_NEAR(timeLeft2, 0.0, 1e-12);
    
    double expected = 0.0 + 5.0 * 0.5;
    EXPECT_NEAR(xEnd1[0], expected, 1e-12);
    EXPECT_NEAR(xEnd2[0], expected, 1e-12);
}

TEST(ODESolverTests, FactoryErrorHandlingEdgeCases) {
    auto solver1 = createODESolver("euler", 1e-10);
    ASSERT_NE(solver1, nullptr);
    EXPECT_DOUBLE_EQ(solver1->getStepSize(), 1e-10);
    
    EXPECT_THROW(createODESolver("Euler"), std::invalid_argument);
    EXPECT_THROW(createODESolver("EULER"), std::invalid_argument);
}

TEST(ODESolverTests, NumericalStabilityWithLargeValues) {
    auto exponentialDecayFunc = [](double t, const std::vector<double>& x, std::vector<double>& xdot) {
        xdot[0] = -0.1 * x[0];
    };
    
    EulerSolver solver(0.01);
    std::vector<double> x0 = {1000.0};
    std::vector<double> xEnd;
    
    double timeLeft = solver.ode(x0, xEnd, 0.0, 5.0, exponentialDecayFunc);
    
    EXPECT_EQ(xEnd.size(), 1);
    EXPECT_NEAR(timeLeft, 0.0, 1e-12);
    
    double expected = 1000.0 * std::exp(-0.5);
    EXPECT_NEAR(xEnd[0], expected, 10.0);
    EXPECT_LT(xEnd[0], x0[0]);
}

TEST(ODESolverTests, FactoryNegativeStepSize) {
    EXPECT_THROW(createODESolver("euler", -0.01), std::invalid_argument);
    EXPECT_THROW(createODESolver("euler", 0.0), std::invalid_argument);
}

TEST(ODESolverTests, ConstructorBoundaryValues) {
    EXPECT_NO_THROW(EulerSolver(1e-15));
    EXPECT_NO_THROW(EulerSolver(1e6));
    EXPECT_THROW(EulerSolver(0.0), std::invalid_argument);
    EXPECT_THROW(EulerSolver(-1e-15), std::invalid_argument);
}