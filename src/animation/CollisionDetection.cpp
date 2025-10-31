#include "animation/CollisionDetection.hpp"
#include <vector>
#include <iostream>
#include <cmath>

namespace animation {

    // Define the missing threshold constant
    const double THRESHOLD = 1e-6;

    // Forward declaration for ODE discontinuous function
    void ode_discontinuous() {
        // This function should signal to the ODE solver that a discontinuity occurred
        // Implementation depends on your ODE solver design
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
    * Return true if bodies are in colliding contact. The
    * parameter 'THRESHOLD' is a small numerical tolerance
    * used for deciding if bodies are colliding.
    */
    bool colliding(Contact *c)
    {
        triple padot = pt_velocity(c->a, c->p), /*  ̇p−a (t0 ) */
            pbdot = pt_velocity(c->b, c->p); /*  ̇p−b (t0 ) */
        double vrel = glm::dot(c->n, (padot - pbdot)); /* v−rel */
        
        // If relative velocity is positive (separating), not colliding
        if(vrel > THRESHOLD) return false;
        
        // If relative velocity is very close to zero, consider it resting contact
        if(vrel > -THRESHOLD) return false;
        
        // If relative velocity is negative (approaching), colliding
        return true;
    }

    void collision(Contact *c, double epsilon)
    {
        triple padot = pt_velocity(c->a, c->p), /*  ̇p−a (t0 ) */
            pbdot = pt_velocity(c->b, c->p), /*  ̇p−b (t0 ) */
            n = c->n, /* ˆn(t0 ) */
            ra = c->p - c->a->x, /* ra */
            rb = c->p - c->b->x; /* rb */
        double vrel = glm::dot(n, (padot - pbdot)), /* v−rel */
            numerator = -(1 + epsilon) * vrel;
        
        /* We'll calculate the denominator in four parts */
        double term1 = 1.0 / c->a->mass,
            term2 = 1.0 / c->b->mass;
        
        // Fix the matrix-vector operations
        // For term3: n · ((I^-1 (ra × n)) × ra)
        triple temp3 = glm::cross(ra, n);
        temp3 = c->a->Iinv * temp3;  // Matrix * vector
        temp3 = glm::cross(temp3, ra);
        double term3 = glm::dot(n, temp3);
        
        // For term4: n · ((I^-1 (rb × n)) × rb)
        triple temp4 = glm::cross(rb, n);
        temp4 = c->b->Iinv * temp4;  // Matrix * vector
        temp4 = glm::cross(temp4, rb);
        double term4 = glm::dot(n, temp4);
        
        /* Compute the impulse magnitude */
        double j = numerator / (term1 + term2 + term3 + term4);
        triple force = static_cast<float>(j) * n;  // Fix scalar multiplication
        
        /* Apply the impulse to the bodies */
        c->a->P += force;
        c->b->P -= force;
        c->a->L += glm::cross(ra, force);
        c->b->L -= glm::cross(rb, force);
        
        /* recompute auxiliary variables */
        c->a->v = c->a->P / static_cast<float>(c->a->mass);  // Fix division
        c->b->v = c->b->P / static_cast<float>(c->b->mass);  // Fix division
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
            for(int i = 0; i < ncontacts; i++) {
                if(colliding(&contacts[i]))
                {
                    collision(&contacts[i], epsilon);
                    had_collision = true;
                    /* Tell the solver we had a collision */
                    ode_discontinuous();
                }
            }
        } while(had_collision == true);
    }
}