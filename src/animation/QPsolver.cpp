#include "animation/QPsolver.hpp"
#include <Eigen/Dense>
#include <osqp.h>
#include <iostream>
#include <vector>

Eigen::VectorXd qp_solve(const Eigen::MatrixXd &A, 
                         const Eigen::VectorXd &b, 
                         const Eigen::VectorXd &f) {
    OSQPInt n = A.rows();
    
    // Convert Eigen matrix to CSC format for OSQP
    // OSQP expects: minimize (1/2) * x^T * P * x + q^T * x
    // So P = A and q = f
    
    std::vector<OSQPFloat> P_data;
    std::vector<OSQPInt> P_indices;
    std::vector<OSQPInt> P_indptr;
    
    // Convert A to upper triangular CSC format (OSQP expects upper triangular)
    P_indptr.push_back(0);
    for (int col = 0; col < n; ++col) {
        for (int row = 0; row <= col; ++row) {
            if (std::abs(A(row, col)) > 1e-12) {
                P_data.push_back(static_cast<OSQPFloat>(A(row, col)));
                P_indices.push_back(static_cast<OSQPInt>(row));
            }
        }
        P_indptr.push_back(static_cast<OSQPInt>(P_data.size()));
    }
    
    // Linear cost vector
    std::vector<OSQPFloat> q(n);
    for (int i = 0; i < n; ++i) {
        q[i] = static_cast<OSQPFloat>(f(i));
    }
    
    // Simple bounds: -b <= x <= b
    std::vector<OSQPFloat> l(n), u(n);
    for (int i = 0; i < n; ++i) {
        l[i] = static_cast<OSQPFloat>(-std::abs(b(i)));
        u[i] = static_cast<OSQPFloat>(std::abs(b(i)));
    }
    
    // Identity matrix for constraints (Ax where A is identity)
    std::vector<OSQPFloat> A_data(n, 1.0);
    std::vector<OSQPInt> A_indices(n);
    std::vector<OSQPInt> A_indptr(n + 1);
    for (int i = 0; i < n; ++i) {
        A_indices[i] = static_cast<OSQPInt>(i);
        A_indptr[i] = static_cast<OSQPInt>(i);
    }
    A_indptr[n] = static_cast<OSQPInt>(n);
    
    // Create CSC matrices
    OSQPCscMatrix* P = (OSQPCscMatrix*)malloc(sizeof(OSQPCscMatrix));
    P->m = n;
    P->n = n;
    P->nz = -1;
    P->nzmax = P_data.size();
    P->x = P_data.data();
    P->i = P_indices.data();
    P->p = P_indptr.data();
    
    OSQPCscMatrix* A_mat = (OSQPCscMatrix*)malloc(sizeof(OSQPCscMatrix));
    A_mat->m = n;
    A_mat->n = n;
    A_mat->nz = -1;
    A_mat->nzmax = n;
    A_mat->x = A_data.data();
    A_mat->i = A_indices.data();
    A_mat->p = A_indptr.data();
    
    // Setup solver
    OSQPSolver* solver = nullptr;
    OSQPSettings* settings = (OSQPSettings*)malloc(sizeof(OSQPSettings));
    
    if (settings) {
        osqp_set_default_settings(settings);
        settings->verbose = 0;
        settings->alpha = 1.0;
    }
    
    OSQPInt exitflag = osqp_setup(&solver, P, q.data(), A_mat, l.data(), u.data(), n, n, settings);
    
    Eigen::VectorXd solution = Eigen::VectorXd::Zero(n);
    
    if (exitflag == 0 && solver != nullptr) {
        // Solve
        osqp_solve(solver);
        
        // Extract solution
        for (int i = 0; i < n; ++i) {
            solution(i) = solver->solution->x[i];
        }
        
        // Cleanup
        osqp_cleanup(solver);
    } else {
        std::cerr << "OSQP setup failed with code: " << exitflag << std::endl;
    }
    
    free(P);
    free(A_mat);
    free(settings);
    
    return solution;
}