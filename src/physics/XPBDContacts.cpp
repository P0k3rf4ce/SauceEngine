#include <physics/XPBD.hpp>
#include <physics/SphereBVH.hpp>
#include <physics/SphereCollider.hpp>
#include <physics/constraints/Constraint.hpp>
#include <physics/constraints/CollisionConstraint.hpp>

#include <app/Entity.hpp>
#include <app/components/MeshRendererComponent.hpp>
#include <app/components/RigidBodyComponent.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>

namespace physics {

namespace {

struct CollisionBody;

struct MeshContact {
  const CollisionBody* bodyA = nullptr;
  const CollisionBody* bodyB = nullptr;
  glm::vec3 pointA = glm::vec3(0.0f);
  glm::vec3 pointB = glm::vec3(0.0f);
  glm::vec3 contactPoint = glm::vec3(0.0f);
  glm::vec3 contactNormal = glm::vec3(0.0f, 0.0f, 1.0f);
  float depth = 0.0f;
};

struct CollisionBody {
  uint32_t index = 0;
  sauce::RigidBodyComponent* rigidBody = nullptr;
  const sauce::modeling::Mesh* mesh = nullptr;
  const SphereBVHNode* bvhRoot = nullptr;
  std::vector<glm::vec3> worldVertices;
  float contactRadius = 0.0f;
};

class MeshCollisionCache {
public:
  static MeshCollisionCache& instance() {
    static MeshCollisionCache cache;
    return cache;
  }

  const SphereBVHNode* getBVHRoot(const sauce::modeling::Mesh* mesh) {
    if (!mesh) {
      return nullptr;
    }

    auto it = bvhRoots.find(mesh);
    if (it == bvhRoots.end()) {
      auto built = SphereBVHNode::fromMesh(*const_cast<sauce::modeling::Mesh*>(mesh));
      it = bvhRoots.emplace(mesh, std::move(built)).first;
    }

    return it->second.get();
  }

  float getContactRadius(const sauce::modeling::Mesh* mesh) {
    if (!mesh) {
      return 0.02f;
    }

    auto it = contactRadii.find(mesh);
    if (it != contactRadii.end()) {
      return it->second;
    }

    const auto& vertices = mesh->getVertices();
    const auto& indices = mesh->getIndices();
    if (vertices.empty() || indices.empty()) {
      return contactRadii.emplace(mesh, 0.02f).first->second;
    }

    double edgeSum = 0.0;
    size_t edgeCount = 0;
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
      const glm::vec3& p0 = vertices[indices[i + 0]].position;
      const glm::vec3& p1 = vertices[indices[i + 1]].position;
      const glm::vec3& p2 = vertices[indices[i + 2]].position;
      edgeSum += glm::length(p1 - p0) + glm::length(p2 - p1) + glm::length(p0 - p2);
      edgeCount += 3;
    }

    const float averageEdgeLength = edgeCount > 0
        ? static_cast<float>(edgeSum / static_cast<double>(edgeCount))
        : 0.02f;
    const float radius = std::clamp(averageEdgeLength * 0.1f, 0.003f, 0.04f);
    return contactRadii.emplace(mesh, radius).first->second;
  }

private:
  std::unordered_map<const sauce::modeling::Mesh*, std::unique_ptr<SphereBVHNode>> bvhRoots;
  std::unordered_map<const sauce::modeling::Mesh*, float> contactRadii;
};

glm::vec3 scalePoint(const glm::vec3& point, const glm::vec3& scale) {
  return point * scale;
}

float maxScaleComponent(const glm::vec3& scale) {
  return std::max({std::abs(scale.x), std::abs(scale.y), std::abs(scale.z)});
}

SphereCollider transformSphere(const SphereCollider& localSphere, const sauce::RigidBodyComponent& body) {
  SphereCollider worldSphere;
  worldSphere.center = body.getPosition() + body.getOrientation() * scalePoint(localSphere.center, body.getScale());
  worldSphere.radius = localSphere.radius * maxScaleComponent(body.getScale());
  return worldSphere;
}

bool spheresOverlap(const SphereCollider& a, const SphereCollider& b) {
  const float radiusSum = a.radius + b.radius;
  return glm::length2(b.center - a.center) <= radiusSum * radiusSum;
}

glm::vec3 closestPointOnTriangle(const glm::vec3& p,
                                 const glm::vec3& a,
                                 const glm::vec3& b,
                                 const glm::vec3& c) {
  const glm::vec3 ab = b - a;
  const glm::vec3 ac = c - a;
  const glm::vec3 ap = p - a;

  const float d1 = glm::dot(ab, ap);
  const float d2 = glm::dot(ac, ap);
  if (d1 <= 0.0f && d2 <= 0.0f) {
    return a;
  }

  const glm::vec3 bp = p - b;
  const float d3 = glm::dot(ab, bp);
  const float d4 = glm::dot(ac, bp);
  if (d3 >= 0.0f && d4 <= d3) {
    return b;
  }

  const float vc = d1 * d4 - d3 * d2;
  if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
    const float v = d1 / (d1 - d3);
    return a + v * ab;
  }

  const glm::vec3 cp = p - c;
  const float d5 = glm::dot(ab, cp);
  const float d6 = glm::dot(ac, cp);
  if (d6 >= 0.0f && d5 <= d6) {
    return c;
  }

  const float vb = d5 * d2 - d1 * d6;
  if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
    const float w = d2 / (d2 - d6);
    return a + w * ac;
  }

  const float va = d3 * d6 - d5 * d4;
  if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
    const float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
    return b + w * (c - b);
  }

  const float denom = 1.0f / (va + vb + vc);
  const float v = vb * denom;
  const float w = vc * denom;
  return a + ab * v + ac * w;
}

void gatherCandidateTriangles(const SphereBVHNode* node,
                              const sauce::RigidBodyComponent& body,
                              const SphereCollider& query,
                              std::vector<uint32_t>& triangleIndices) {
  if (!node) {
    return;
  }

  const SphereCollider worldNode = transformSphere(node->sphere, body);
  if (!spheresOverlap(worldNode, query)) {
    return;
  }

  if (node->isLeaf()) {
    triangleIndices.insert(triangleIndices.end(), node->triangleIndices.begin(), node->triangleIndices.end());
    return;
  }

  gatherCandidateTriangles(node->left.get(), body, query, triangleIndices);
  gatherCandidateTriangles(node->right.get(), body, query, triangleIndices);
}

void generateVertexContacts(const CollisionBody& sampleBody,
                            const CollisionBody& targetBody,
                            std::vector<MeshContact>& contacts) {
  if (!sampleBody.bvhRoot || !targetBody.bvhRoot || !sampleBody.mesh || !targetBody.mesh) {
    return;
  }

  const auto& targetIndices = targetBody.mesh->getIndices();
  const glm::vec3 targetCenter = targetBody.rigidBody->getWorldCenterOfMass();

  for (size_t vertexIndex = 0; vertexIndex < sampleBody.worldVertices.size(); ++vertexIndex) {
    const glm::vec3& worldVertex = sampleBody.worldVertices[vertexIndex];
    SphereCollider query;
    query.center = worldVertex;
    query.radius = sampleBody.contactRadius;

    std::vector<uint32_t> candidateTriangles;
    gatherCandidateTriangles(targetBody.bvhRoot, *targetBody.rigidBody, query, candidateTriangles);
    if (candidateTriangles.empty()) {
      continue;
    }

    std::sort(candidateTriangles.begin(), candidateTriangles.end());
    candidateTriangles.erase(std::unique(candidateTriangles.begin(), candidateTriangles.end()),
                             candidateTriangles.end());

    float bestDistanceSq = std::numeric_limits<float>::infinity();
    glm::vec3 bestPoint(0.0f);
    glm::vec3 bestNormal(0.0f, 0.0f, 1.0f);

    for (uint32_t triangleIndex : candidateTriangles) {
      const size_t base = static_cast<size_t>(triangleIndex) * 3;
      if (base + 2 >= targetIndices.size()) {
        continue;
      }

      const glm::vec3& t0 = targetBody.worldVertices[targetIndices[base + 0]];
      const glm::vec3& t1 = targetBody.worldVertices[targetIndices[base + 1]];
      const glm::vec3& t2 = targetBody.worldVertices[targetIndices[base + 2]];
      const glm::vec3 closestPoint = closestPointOnTriangle(worldVertex, t0, t1, t2);
      const glm::vec3 delta = worldVertex - closestPoint;
      const float distanceSq = glm::dot(delta, delta);
      if (distanceSq > sampleBody.contactRadius * sampleBody.contactRadius ||
          distanceSq >= bestDistanceSq) {
        continue;
      }

      glm::vec3 normal;
      if (distanceSq > 1e-10f) {
        normal = delta / std::sqrt(distanceSq);
      } else {
        normal = glm::cross(t1 - t0, t2 - t0);
        const float normalLength = glm::length(normal);
        if (normalLength <= 1e-8f) {
          continue;
        }
        normal /= normalLength;
        if (glm::dot(sampleBody.rigidBody->getWorldCenterOfMass() - targetCenter,
                     normal) < 0.0f) {
          normal = -normal;
        }
      }

      bestDistanceSq = distanceSq;
      bestPoint = closestPoint;
      bestNormal = normal;
    }

    if (!std::isfinite(bestDistanceSq)) {
      continue;
    }

    const float bestDistance = std::sqrt(std::max(bestDistanceSq, 0.0f));
    contacts.push_back({
      &sampleBody,
      &targetBody,
      worldVertex,
      bestPoint,
      0.5f * (worldVertex + bestPoint),
      bestNormal,
      sampleBody.contactRadius - bestDistance,
    });
  }
}

void reduceContacts(std::vector<MeshContact>& contacts) {
  constexpr size_t kMaxContacts = 8;
  constexpr float kMinContactSpacingSq = 0.0004f;
  constexpr float kMinPenetrationDepth = 0.0015f;

  contacts.erase(
      std::remove_if(contacts.begin(), contacts.end(), [](const MeshContact& contact) {
        return contact.depth <= kMinPenetrationDepth;
      }),
      contacts.end());

  if (contacts.size() <= kMaxContacts) {
    return;
  }

  std::sort(contacts.begin(), contacts.end(), [](const MeshContact& a, const MeshContact& b) {
    return a.depth > b.depth;
  });

  std::vector<MeshContact> reduced;
  reduced.reserve(kMaxContacts);
  for (const auto& contact : contacts) {
    bool tooClose = false;
    for (const auto& kept : reduced) {
      if (glm::length2(contact.contactPoint - kept.contactPoint) < kMinContactSpacingSq) {
        tooClose = true;
        break;
      }
    }
    if (!tooClose) {
      reduced.push_back(contact);
    }
    if (reduced.size() == kMaxContacts) {
      break;
    }
  }

  if (reduced.empty()) {
    reduced.push_back(contacts.front());
  }
  contacts = std::move(reduced);
}

bool tryBuildCollisionBody(uint32_t index,
                           sauce::RigidBodyComponent* rigidBody,
                           CollisionBody& collisionBody) {
  if (!rigidBody || !rigidBody->isCollisionEnabled()) {
    return false;
  }

  auto* owner = rigidBody->getOwner();
  if (!owner) {
    return false;
  }

  auto* meshComp = owner->getComponent<sauce::MeshRendererComponent>();
  if (!meshComp || !meshComp->getMesh()) {
    return false;
  }

  const auto& mesh = meshComp->getMesh();
  const auto& vertices = mesh->getVertices();
  if (vertices.empty()) {
    return false;
  }

  const glm::vec3 bodyScale = rigidBody->getScale();
  std::vector<glm::vec3> worldVertices;
  worldVertices.reserve(vertices.size());
  for (const auto& vertex : vertices) {
    worldVertices.push_back(
        rigidBody->getPosition() + rigidBody->getOrientation() * scalePoint(vertex.position, bodyScale));
  }

  collisionBody = CollisionBody{
    index,
    rigidBody,
    mesh.get(),
    MeshCollisionCache::instance().getBVHRoot(mesh.get()),
    std::move(worldVertices),
    MeshCollisionCache::instance().getContactRadius(mesh.get()) * maxScaleComponent(bodyScale),
  };
  return true;
}

void appendContactConstraint(std::vector<std::unique_ptr<Constraint>>& constraints,
                             const MeshContact& contact) {
  const glm::vec3 centerA = contact.bodyA->rigidBody->getWorldCenterOfMass();
  const glm::vec3 centerB = contact.bodyB->rigidBody->getWorldCenterOfMass();
  const glm::quat invOrientationA = glm::inverse(contact.bodyA->rigidBody->getOrientation());
  const glm::quat invOrientationB = glm::inverse(contact.bodyB->rigidBody->getOrientation());
  const glm::vec3 localOffsetA = invOrientationA * (contact.pointA - centerA);
  const glm::vec3 localOffsetB = invOrientationB * (contact.pointB - centerB);

  constraints.push_back(std::make_unique<CollisionConstraint>(
      contact.bodyA->index,
      contact.bodyB->index,
      contact.contactNormal,
      contact.depth,
      localOffsetA,
      localOffsetB,
      0.0f));
}

void appendReducedContacts(const CollisionBody& sampleBody,
                           const CollisionBody& targetBody,
                           std::vector<MeshContact>& contacts) {
  if (!sampleBody.rigidBody->isDynamic()) {
    return;
  }

  std::vector<MeshContact> directionalContacts;
  generateVertexContacts(sampleBody, targetBody, directionalContacts);
  reduceContacts(directionalContacts);
  contacts.insert(contacts.end(), directionalContacts.begin(), directionalContacts.end());
}

} // namespace

std::vector<std::unique_ptr<Constraint>> XPBDSolver::generateCollisionConstraints(
    const std::vector<sauce::RigidBodyComponent*>& rigidBodies) {
  std::vector<std::unique_ptr<Constraint>> constraints;

  std::vector<CollisionBody> bodies;
  bodies.reserve(rigidBodies.size());

  for (uint32_t i = 0; i < static_cast<uint32_t>(rigidBodies.size()); ++i) {
    CollisionBody body;
    if (tryBuildCollisionBody(i, rigidBodies[i], body)) {
      bodies.push_back(std::move(body));
    }
  }

  for (size_t i = 0; i < bodies.size(); ++i) {
    for (size_t j = i + 1; j < bodies.size(); ++j) {
      if (!bodies[i].bvhRoot || !bodies[j].bvhRoot) {
        continue;
      }

      SphereCollider broadA = transformSphere(bodies[i].bvhRoot->sphere, *bodies[i].rigidBody);
      SphereCollider broadB = transformSphere(bodies[j].bvhRoot->sphere, *bodies[j].rigidBody);
      broadA.radius += bodies[i].contactRadius;
      broadB.radius += bodies[j].contactRadius;
      if (!spheresOverlap(broadA, broadB)) {
        continue;
      }

      std::vector<MeshContact> contacts;
      appendReducedContacts(bodies[i], bodies[j], contacts);
      appendReducedContacts(bodies[j], bodies[i], contacts);

      for (const auto& contact : contacts) {
        if (contact.depth <= 0.0f || !contact.bodyA || !contact.bodyB) {
          continue;
        }

        if (!contact.bodyB->rigidBody->isDynamic()) {
          recordStaticContact(contact.bodyA->index, contact.contactNormal);
        }
        if (!contact.bodyA->rigidBody->isDynamic()) {
          recordStaticContact(contact.bodyB->index, -contact.contactNormal);
        }
        appendContactConstraint(constraints, contact);
      }
    }
  }

  return constraints;
}

} // namespace physics
