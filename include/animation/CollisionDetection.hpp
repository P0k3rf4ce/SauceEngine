#ifndef COLLISION_DETECTION_HPP
#define COLLISION_DETECTION_HPP

#include <glm/glm.hpp>
#include <vector>
#include "ODEsolver.hpp"

namespace animation {

    // Type definitions for physics simulation
    typedef glm::vec3 triple;

    // Forward declaration for rigid body structure
    struct RigidBody {
        double mass;           // mass M
        triple x;             // position
        triple v;             // linear velocity
        triple omega;         // angular velocity
        triple P;             // linear momentum
        triple L;             // angular momentum
        glm::mat3 Iinv;       // inverse inertia tensor
    };

    // Contact structure as defined in collision.txt
    struct Contact {
        RigidBody *a,         /* body containing vertex */
                  *b;         /* body containing face */
        triple p,             /* world-space vertex location */
               n,             /* outwards pointing normal of face */
               ea,            /* edge direction for A */
               eb;            /* edge direction for B */
        bool vf;              /* true if vertex/face contact */
    };

    // Global variables for collision detection
    extern int ncontacts;
    extern const double THRESHOLD; /* collision threshold */
    extern std::vector<Contact> contacts; /* array of contacts */

    // Function declarations
    triple pt_velocity(RigidBody *body, triple p);
    bool colliding(Contact *c);
    void collision(Contact *c, double epsilon);
    void FindAllCollisions(std::vector<Contact> &contacts, int ncontacts);
    void ode_discontinuous();

}

#endif // COLLISION_DETECTION_HPP