#pragma once

#include "app/Component.hpp"
#include "app/modeling/Mesh.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>

namespace sauce {

class RigidBodyComponent : public Component {
public:
  RigidBodyComponent(
      glm::vec3 initPosition,
      glm::vec3 initVelocity,
      glm::quat initOrientation,
      glm::vec3 initAngularVelocity,
      glm::vec3 accumulatedForce = glm::vec3(0.0f, 0.0f, 0.0f),
      float invMass = 1.0f,
      glm::mat3 invInertiaTensor = glm::mat3(1.0f),
      glm::vec3 initScale = glm::vec3(1.0f)
      ) : 
    position(initPosition), velocity(initVelocity),
    orientation(initOrientation), angularVelocity(initAngularVelocity), accumulatedForce(accumulatedForce),
    invMass(invMass), invInertiaTensor(invInertiaTensor),
    storedInvMass(invMass), storedInvInertiaTensor(invInertiaTensor),
    scale(initScale) {}

  bool             isDynamic()                      const { return invMass > 1e-8f; }
  bool             canBeDynamic()                   const { return storedInvMass > 1e-8f; }
  bool             isSleeping()                     const { return sleeping; }

  const glm::vec3& getPosition()                    const { return position; }
  const glm::vec3& getCenterOfMass()                const { return centerOfMass; }
  const glm::vec3& getScale()                       const { return scale; }
  glm::vec3        getWorldCenterOfMass()           const { return position + orientation * (centerOfMass * scale); }
  const glm::vec3& getVelocity()                    const { return velocity; }
  const glm::quat& getOrientation()                 const { return orientation; }
  const glm::vec3& getAngularVelocity()             const { return angularVelocity; }
  const glm::vec3& getAccumulatedForce()            const { return accumulatedForce; }
  const glm::vec3& getExternalForces()              const { return accumulatedForce; }
  bool             isCollisionEnabled()             const { return collisionEnabled; }
  float            getInvMass()                     const { return invMass; }
  const glm::mat3& getInvInertiaTensor()            const { return invInertiaTensor; }

  void setPosition(const glm::vec3& p)              { position = p; }
  void setCenterOfMass(const glm::vec3& p)          { centerOfMass = p; }
  void setScale(const glm::vec3& s)                 { scale = s; }
  void setWorldCenterOfMass(const glm::vec3& p)     { position = p - orientation * (centerOfMass * scale); }
  void setVelocity(const glm::vec3& v)              { velocity = v; }
  void setOrientation(const glm::quat& q)           { orientation = q; }
  void setAngularVelocity(const glm::vec3& w)       { angularVelocity = w; }
  void setAccumulatedForce(const glm::vec3& force)  { accumulatedForce = force; }
  void setExternalForces(const glm::vec3& force)    { accumulatedForce = force; }
  void setCollisionEnabled(bool enabled)            { collisionEnabled = enabled; }
  void setInvMass(float w)                          { invMass = w; }
  void setInvInertiaTensor(const glm::mat3& I)      { invInertiaTensor = I; }
  void setMassProperties(float invMassValue, const glm::mat3& invInertiaTensorValue) {
    invMass = invMassValue;
    invInertiaTensor = invInertiaTensorValue;
    storedInvMass = invMassValue;
    storedInvInertiaTensor = invInertiaTensorValue;
    sleeping = false;
  }
  void clearAccumulatedForce()                      { accumulatedForce = glm::vec3(0.0f); }
  void clearExternalForces()                        { accumulatedForce = glm::vec3(0.0f); }
  void addForce(const glm::vec3& force)             { accumulatedForce += force; }
  void offsetExternalForce(glm::vec3 force)         { accumulatedForce += force; }
  void sleep() {
    if (!canBeDynamic()) {
      return;
    }
    sleeping = true;
    invMass = 0.0f;
    invInertiaTensor = glm::mat3(0.0f);
    velocity = glm::vec3(0.0f);
    angularVelocity = glm::vec3(0.0f);
    clearAccumulatedForce();
  }
  void wake() {
    if (!sleeping || !canBeDynamic()) {
      return;
    }
    sleeping = false;
    invMass = storedInvMass;
    invInertiaTensor = storedInvInertiaTensor;
  }

  // approximate center of mass given a mesh
  static glm::vec3 meshCenterOfMass(std::shared_ptr<modeling::Mesh> m);
  // approximate inverse mass given a mesh
  static float meshInvMass(std::shared_ptr<modeling::Mesh> m);
  static float scaledMeshInvMass(std::shared_ptr<modeling::Mesh> m, const glm::vec3& scale);
  // approximate inverse inertia tensor using the mesh bounds around center of mass
  static glm::mat3 meshInvInertiaTensor(std::shared_ptr<modeling::Mesh> m,
                                        const glm::vec3& centerOfMass,
                                        float invMass);
  static glm::mat3 meshInvInertiaTensor(std::shared_ptr<modeling::Mesh> m,
                                        const glm::vec3& centerOfMass,
                                        float invMass,
                                        const glm::vec3& scale);

  // No implementation for this for now
  virtual void render() override {};

  // Predict rigid-body motion from accumulated forces with no regard for constraints.
  virtual void update(float deltatime) override;
  virtual ~RigidBodyComponent() = default;

private:
  glm::vec3 position;
  glm::vec3 centerOfMass;
  glm::vec3 velocity;
  glm::quat orientation;
  glm::vec3 angularVelocity;

  glm::vec3 accumulatedForce;
  bool collisionEnabled = true;
  bool sleeping = false;

  float invMass;
  glm::mat3 invInertiaTensor;
  float storedInvMass = 0.0f;
  glm::mat3 storedInvInertiaTensor = glm::mat3(0.0f);
  glm::vec3 scale = glm::vec3(1.0f);
};

}
