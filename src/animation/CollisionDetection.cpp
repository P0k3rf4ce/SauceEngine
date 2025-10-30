#include "animation/CollisionDetection.hpp"
#include <vector>
#include <iostream>
#include <cmath>
#include <algorithm>
#include "animation/ODEsolver.hpp"  // for ODESolver::getStepSize/setStepSize

namespace animation {

const double THRESHOLD = 0.01;
// Keep a weak reference to the solver (owned elsewhere)
static ODESolver* g_active_solver = nullptr;

void register_active_solver(ODESolver* solver) {
    g_active_solver = solver;
}

// This function signals a discontinuity to the ODE solver.
// Strategy: reduce the step size to improve stability after an impulse.
void ode_discontinuous() {
    if (!g_active_solver) {
        return;
    }
    try {
        double h = g_active_solver->getStepSize();
        // Reduce step by half, with a small lower bound to avoid zero/negative.
        double newH = std::max(h * 0.5, 1e-8);
        g_active_solver->setStepSize(newH);
    } catch (...) {
        // Be robust: never throw from here
    }
}

/*
* Operators: if 'x' and 'y' are triples,
* assume that 'x ^ y' is their cross product,
* and 'x * y' is their dot product.
*/
/* Return the velocity of a point on a rigid body */
triple pt_velocity(RigidBody *body, triple p)
{
    return body->v + glm::cross(body->omega, (p - body->x));
}

/*
* Return true if bodies are in colliding contact.
* Bodies are colliding if they are approaching or stationary (not separating).
* The parameter 'THRESHOLD' is a small numerical tolerance.
*/
bool colliding(Contact *c)
{
    triple padot = pt_velocity(c->a, c->p), /*  ̇p−a (t0 ) */
        pbdot = pt_velocity(c->b, c->p); /*  ̇p−b (t0 ) */
    double vrel = glm::dot(c->n, (padot - pbdot)); /* v−rel */
    
    // If relative velocity is positive (separating), not colliding
    if (vrel > THRESHOLD) {
        return false;
    }
    
    // If relative velocity is negative or near zero (approaching or resting), colliding
    return true;
}

void collision(Contact *c, double epsilon)
{
    triple padot = pt_velocity(c->a, c->p),
        pbdot = pt_velocity(c->b, c->p),
        n = c->n,
        ra = c->p - c->a->x,
        rb = c->p - c->b->x;
    double vrel = glm::dot(n, (padot - pbdot));
    double numerator = -(1 + epsilon) * vrel;

    double term1 = 1.0 / c->a->mass;
    double term2 = 1.0 / c->b->mass;

    triple temp3 = glm::cross(ra, n);
    temp3 = c->a->Iinv * temp3;
    temp3 = glm::cross(temp3, ra);
    double term3 = glm::dot(n, temp3);

    triple temp4 = glm::cross(rb, n);
    temp4 = c->b->Iinv * temp4;
    temp4 = glm::cross(temp4, rb);
    double term4 = glm::dot(n, temp4);

    double j = numerator / (term1 + term2 + term3 + term4);
    triple force = static_cast<float>(j) * n;

    c->a->P += force;
    c->b->P -= force;
    c->a->L += glm::cross(ra, force);
    c->b->L -= glm::cross(rb, force);

    c->a->v = c->a->P / static_cast<float>(c->a->mass);
    c->b->v = c->b->P / static_cast<float>(c->b->mass);
    c->a->omega = c->a->Iinv * c->a->L;
    c->b->omega = c->b->Iinv * c->b->L;
}

/**
* @brief Detect collisions between rigid bodies and generate contact information
*   
* @param contacts Output vector to store detected contacts
*/
void FindAllCollisions(std::vector<Contact> &contacts, int ncontacts)
{
    bool had_collision;
    double epsilon = 0.5;
    do {
        had_collision = false;
        for (int i = 0; i < ncontacts; i++) {
            if (colliding(&contacts[i])) {
                collision(&contacts[i], epsilon);
                had_collision = true;
                ode_discontinuous(); // notify solver
            }
        }
    } while (had_collision == true);
}
} // namespace animation