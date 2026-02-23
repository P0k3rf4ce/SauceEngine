#pragma once

#include "app/Component.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace sauce {

class RigidBodyComponent : public Component {
public:
  
  RigidBodyComponent(
      glm::vec3 initPosition,
      glm::vec3 initVelocity,
      glm::quat initOrientation,
      glm::vec3 initAngularVelocity,
      glm::vec3 externalForces = glm::vec3(0.0f, 0.0f, 0.0f),
      float invMass = 1.0f,
      glm::mat3 invInertiaTensor = glm::mat3(1.0f)
      ) : 
    position(initPosition), velocity(initVelocity),
    orientation(initOrientation), angularVelocity(initAngularVelocity), externalForces(externalForces),
    invMass(invMass), invInertiaTensor(invInertiaTensor) {}

  void offsetExternalForce(glm::vec3 force) {
    externalForces += force;
  }

  const glm::vec3& getPosition()                    const { return position; }
  const glm::vec3& getCenterOfMass()                const { return centerOfMass; }
  const glm::vec3& getVelocity()                    const { return velocity; }
  const glm::quat& getOrientation()                 const { return orientation; }
  const glm::vec3& getAngularVelocity()             const { return angularVelocity; }
  const glm::vec3& getExternalForces()              const { return externalForces; }
  float            getInvMass()                     const { return invMass; }
  const glm::mat3& getInvInertiaTensor()            const { return invInertiaTensor; }

  void setPosition(const glm::vec3& p)              { position = p; }
  void setCenterOfMass(const glm::vec3& p)          { centerOfMass = p; }
  void setVelocity(const glm::vec3& v)              { velocity = v; }
  void setOrientation(const glm::quat& q)           { orientation = q; }
  void setAngularVelocity(const glm::vec3& w)       { angularVelocity = w; }
  void setExternalForces(const glm::vec3& f)        { externalForces = f; }
  void setInvMass(float w)                          { invMass = w; }
  void setInvInertiaTensor(const glm::mat3& I)      { invInertiaTensor = I; }
  void clearExternalForces()                        { externalForces = glm::vec3(0.0f); }

  // No implementation for this for now
  virtual void render() override;

  // Moves the object using its external forces with no regard for constraints.
  virtual void update(float deltatime) override;
  virtual ~RigidBodyComponent() = default;

private:
  glm::vec3 position;
  glm::vec3 centerOfMass;
  glm::vec3 velocity;
  glm::quat orientation;
  glm::vec3 angularVelocity;

  glm::vec3 externalForces;

  float invMass;
  glm::mat3 invInertiaTensor;
};

}

