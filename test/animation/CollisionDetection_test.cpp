#include <gtest/gtest.h>
#include <vector>
#include "animation/CollisionDetection.hpp"
#include <cmath>

// Test helper functions and mock data
using namespace animation;

// Helper to create a test rigid body
RigidBody createTestBody(triple pos, triple vel, triple omega, double mass) {
    RigidBody body;
    body.x = pos;
    body.v = vel;
    body.omega = omega;
    body.mass = mass;
    body.P = vel * static_cast<float>(mass);
    body.L = triple(0.0f, 0.0f, 0.0f);
    // Simple identity matrix for inverse inertia
    body.Iinv = glm::mat3(1.0f / mass);
    return body;
}

// Helper to create a test contact
Contact createTestContact(RigidBody* a, RigidBody* b, triple point, triple normal) {
    Contact contact;
    contact.a = a;
    contact.b = b;
    contact.p = point;
    contact.n = normal;
    return contact;
}


TEST(CollisionDetectionTest, PtVelocityStaticBody) {
    RigidBody body = createTestBody(
        triple(0.0f, 0.0f, 0.0f),  // position
        triple(0.0f, 0.0f, 0.0f),  // velocity
        triple(0.0f, 0.0f, 0.0f),  // angular velocity
        1.0                         // mass
    );
    
    triple point(1.0f, 0.0f, 0.0f);
    triple velocity = animation::pt_velocity(&body, point);
    
    EXPECT_FLOAT_EQ(velocity.x, 0.0f);
    EXPECT_FLOAT_EQ(velocity.y, 0.0f);
    EXPECT_FLOAT_EQ(velocity.z, 0.0f);
}

TEST(CollisionDetectionTest, PtVelocityLinearMotion) {
    RigidBody body = createTestBody(
        triple(0.0f, 0.0f, 0.0f),  // position
        triple(1.0f, 2.0f, 3.0f),  // velocity
        triple(0.0f, 0.0f, 0.0f),  // angular velocity
        1.0                         // mass
    );
    
    triple point(1.0f, 0.0f, 0.0f);
    triple velocity = animation::pt_velocity(&body, point);
    
    EXPECT_FLOAT_EQ(velocity.x, 1.0f);
    EXPECT_FLOAT_EQ(velocity.y, 2.0f);
    EXPECT_FLOAT_EQ(velocity.z, 3.0f);
}

TEST(CollisionDetectionTest, PtVelocityRotationalMotion) {
    RigidBody body = createTestBody(
        triple(0.0f, 0.0f, 0.0f),  // position
        triple(0.0f, 0.0f, 0.0f),  // velocity
        triple(0.0f, 0.0f, 1.0f),  // angular velocity (rotating around z-axis)
        1.0                         // mass
    );
    
    triple point(1.0f, 0.0f, 0.0f);  // Point on x-axis
    triple velocity = animation::pt_velocity(&body, point);
    
    // Expected: cross product of (0,0,1) x (1,0,0) = (0,1,0)
    EXPECT_FLOAT_EQ(velocity.x, 0.0f);
    EXPECT_FLOAT_EQ(velocity.y, 1.0f);
    EXPECT_FLOAT_EQ(velocity.z, 0.0f);
}

TEST(CollisionDetectionTest, CollidingNearThreshold) {
    animation::RigidBody bodyA = createTestBody(
        animation::triple(0.0f, 0.0f, 0.0f),
        animation::triple(-5e-5f, 0.0f, 0.0f),  // A moves away from B
        animation::triple(0.0f, 0.0f, 0.0f),
        1.0
    );
    
    animation::RigidBody bodyB = createTestBody(
        animation::triple(2.0f, 0.0f, 0.0f),
        animation::triple(3e-5f, 0.0f, 0.0f),   // B moves away from A
        animation::triple(0.0f, 0.0f, 0.0f),
        1.0
    );
    
    animation::Contact contact = createTestContact(&bodyA, &bodyB, 
        animation::triple(1.0f, 0.0f, 0.0f),    // contact point
        animation::triple(1.0f, 0.0f, 0.0f));   // normal pointing from A to B
    
    // vrel = (1,0,0) · ((-5e-5,0,0) - (3e-5,0,0)) = (1,0,0) · (-8e-5,0,0) = -8e-5
    // vrel = -8e-5 = -0.00008 < -THRESHOLD = -0.000001, so collision detected
    EXPECT_TRUE(animation::colliding(&contact));
}

TEST(CollisionDetectionTest, CollidingSeparating) {
    animation::RigidBody bodyA = createTestBody(
        animation::triple(0.0f, 0.0f, 0.0f),
        animation::triple(2.0f, 0.0f, 0.0f),    // A moves faster towards B direction
        animation::triple(0.0f, 0.0f, 0.0f),
        1.0
    );
    
    animation::RigidBody bodyB = createTestBody(
        animation::triple(2.0f, 0.0f, 0.0f),
        animation::triple(1.0f, 0.0f, 0.0f),    // B moves slower in same direction
        animation::triple(0.0f, 0.0f, 0.0f),
        1.0
    );
    
    animation::Contact contact = createTestContact(&bodyA, &bodyB, 
        animation::triple(1.0f, 0.0f, 0.0f),
        animation::triple(1.0f, 0.0f, 0.0f));
    
    // vrel = (1,0,0) · ((2,0,0) - (1,0,0)) = (1,0,0) · (1,0,0) = 1
    // vrel = 1 > THRESHOLD = 1e-6, so bodies are separating (no collision)
    EXPECT_FALSE(animation::colliding(&contact));
}

TEST(CollisionDetectionTest, CollisionImpulseApplication) {
    RigidBody bodyA = createTestBody(
        triple(0.0f, 0.0f, 0.0f),
        triple(1.0f, 0.0f, 0.0f),    // Moving towards collision
        triple(0.0f, 0.0f, 0.0f),
        1.0
    );
    
    RigidBody bodyB = createTestBody(
        triple(2.0f, 0.0f, 0.0f),
        triple(-1.0f, 0.0f, 0.0f),   // Moving towards collision
        triple(0.0f, 0.0f, 0.0f),
        1.0
    );
    
    Contact contact = createTestContact(&bodyA, &bodyB, 
        triple(1.0f, 0.0f, 0.0f),
        triple(1.0f, 0.0f, 0.0f));
    
    // Store initial momentum
    triple initialMomentumA = bodyA.P;
    triple initialMomentumB = bodyB.P;
    triple totalInitialMomentum = initialMomentumA + initialMomentumB;
    
    animation::collision(&contact, 0.5);  // 50% restitution
    
    // Check momentum conservation
    triple totalFinalMomentum = bodyA.P + bodyB.P;
    EXPECT_NEAR(totalFinalMomentum.x, totalInitialMomentum.x, 1e-5);
    EXPECT_NEAR(totalFinalMomentum.y, totalInitialMomentum.y, 1e-5);
    EXPECT_NEAR(totalFinalMomentum.z, totalInitialMomentum.z, 1e-5);
}

TEST(CollisionDetectionTest, CollisionVelocityUpdate) {
    RigidBody bodyA = createTestBody(
        triple(0.0f, 0.0f, 0.0f),
        triple(2.0f, 0.0f, 0.0f),
        triple(0.0f, 0.0f, 0.0f),
        1.0
    );
    
    RigidBody bodyB = createTestBody(
        triple(2.0f, 0.0f, 0.0f),
        triple(0.0f, 0.0f, 0.0f),    // Initially at rest
        triple(0.0f, 0.0f, 0.0f),
        1.0
    );
    
    Contact contact = createTestContact(&bodyA, &bodyB, 
        triple(1.0f, 0.0f, 0.0f),
        triple(1.0f, 0.0f, 0.0f));
    
    animation::collision(&contact, 1.0);  // Perfect elastic collision
    
    // In elastic collision between equal masses, velocities should exchange
    EXPECT_NEAR(bodyA.v.x, 0.0f, 1e-5);
    EXPECT_NEAR(bodyB.v.x, 2.0f, 1e-5);
}

TEST(CollisionDetectionTest, FindAllCollisionsNoCollisions) {
    animation::RigidBody bodyA = createTestBody(
        animation::triple(0.0f, 0.0f, 0.0f),
        animation::triple(2.0f, 0.0f, 0.0f),    // Moving faster towards B direction
        animation::triple(0.0f, 0.0f, 0.0f),
        1.0
    );
    
    animation::RigidBody bodyB = createTestBody(
        animation::triple(2.0f, 0.0f, 0.0f),
        animation::triple(1.0f, 0.0f, 0.0f),    // Moving slower in same direction
        animation::triple(0.0f, 0.0f, 0.0f),
        1.0
    );
    
    std::vector<animation::Contact> contacts;
    contacts.push_back(createTestContact(&bodyA, &bodyB, 
        animation::triple(1.0f, 0.0f, 0.0f),
        animation::triple(1.0f, 0.0f, 0.0f)));
    
    animation::triple initialVelA = bodyA.v;
    animation::triple initialVelB = bodyB.v;
    
    animation::FindAllCollisions(contacts, 1);
    
    // Velocities should remain unchanged since bodies are separating
    // vrel = (1,0,0) · ((2,0,0) - (1,0,0)) = 1 > THRESHOLD, so no collision
    EXPECT_FLOAT_EQ(bodyA.v.x, initialVelA.x);
    EXPECT_FLOAT_EQ(bodyB.v.x, initialVelB.x);
}

TEST(CollisionDetectionTest, FindAllCollisionsWithCollision) {
    RigidBody bodyA = createTestBody(
        triple(0.0f, 0.0f, 0.0f),
        triple(1.0f, 0.0f, 0.0f),    // Moving towards collision
        triple(0.0f, 0.0f, 0.0f),
        1.0
    );
    
    RigidBody bodyB = createTestBody(
        triple(2.0f, 0.0f, 0.0f),
        triple(-1.0f, 0.0f, 0.0f),   // Moving towards collision
        triple(0.0f, 0.0f, 0.0f),
        1.0
    );
    
    std::vector<Contact> contacts;
    contacts.push_back(createTestContact(&bodyA, &bodyB, 
        triple(1.0f, 0.0f, 0.0f),
        triple(-1.0f, 0.0f, 0.0f)));  // Normal pointing from B towards A
    
    animation::FindAllCollisions(contacts, 1);
    
    // With epsilon = 0.5 in FindAllCollisions, not perfectly elastic
    // The relative velocity magnitude will be reduced by the restitution coefficient
    // Initial relative velocity magnitude: 2.0, after collision: 0.5 * 2.0 = 1.0
    // Since collision reverses direction: final relative velocity = -1.0
    animation::triple relativeVelocity = bodyA.v - bodyB.v;
    EXPECT_FLOAT_EQ(relativeVelocity.x, -1.0f);  // Reduced by restitution coefficient
}

TEST(CollisionDetectionTest, MultipleCollisions) {
    RigidBody bodyA = createTestBody(triple(-1.0f, 0.0f, 0.0f), triple(1.0f, 0.0f, 0.0f), triple(0.0f, 0.0f, 0.0f), 1.0);
    RigidBody bodyB = createTestBody(triple(0.0f, 0.0f, 0.0f), triple(0.0f, 0.0f, 0.0f), triple(0.0f, 0.0f, 0.0f), 1.0);
    RigidBody bodyC = createTestBody(triple(1.0f, 0.0f, 0.0f), triple(-1.0f, 0.0f, 0.0f), triple(0.0f, 0.0f, 0.0f), 1.0);
    
    std::vector<Contact> contacts;
    contacts.push_back(createTestContact(&bodyA, &bodyB, triple(-0.5f, 0.0f, 0.0f), triple(1.0f, 0.0f, 0.0f)));
    contacts.push_back(createTestContact(&bodyB, &bodyC, triple(0.5f, 0.0f, 0.0f), triple(1.0f, 0.0f, 0.0f)));
    
    triple totalInitialMomentum = bodyA.P + bodyB.P + bodyC.P;
    
    animation::FindAllCollisions(contacts, 2);
    
    triple totalFinalMomentum = bodyA.P + bodyB.P + bodyC.P;
    EXPECT_NEAR(totalFinalMomentum.x, totalInitialMomentum.x, 1e-5);
}