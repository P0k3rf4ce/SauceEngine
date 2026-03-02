#include <algorithm>
#include <app/modeling/Mesh.hpp>
#include <physics/Collider.hpp>
#include <physics/SphereBVH.hpp>
#include <physics/SphereCollider.hpp>
#include <vector>
#include <glm/gtx/norm.hpp>

namespace physics {

static constexpr size_t MAX_MANIFOLD_CONTACTS = 4;

// ── helpers ──────────────────────────────────────────────────────────

static bool spheresOverlap(const SphereCollider& a, const SphereCollider& b) {
    float radiusSum = a.radius + b.radius;
    return glm::length2(b.center - a.center) < radiusSum * radiusSum;
}

// Reduces a set of contacts down to at most MAX_MANIFOLD_CONTACTS that
// best represent the contact surface. Strategy:
//   1. Keep the deepest penetration (most important for stability)
//   2. Keep the point farthest from #1 (maximise spread)
//   3. Keep the point that maximises triangle area with #1 and #2
//   4. Keep the point that maximises quadrilateral area with #1, #2, #3
static void reduceManifold(std::vector<ContactInfo>& contacts) {
    if (contacts.size() <= MAX_MANIFOLD_CONTACTS) return;

    std::vector<ContactInfo> reduced;
    reduced.reserve(MAX_MANIFOLD_CONTACTS);

    // 1. Deepest penetration
    size_t deepestIdx = 0;
    for (size_t i = 1; i < contacts.size(); ++i) {
        if (contacts[i].depth > contacts[deepestIdx].depth) {
            deepestIdx = i;
        }
    }
    reduced.push_back(contacts[deepestIdx]);

    // 2. Farthest from the deepest
    size_t farthestIdx = 0;
    float maxDistSq = -1.0f;
    for (size_t i = 0; i < contacts.size(); ++i) {
        float dSq = glm::length2(contacts[i].contactPoint - reduced[0].contactPoint);
        if (dSq > maxDistSq) {
            maxDistSq = dSq;
            farthestIdx = i;
        }
    }
    reduced.push_back(contacts[farthestIdx]);

    // 3. Maximise triangle area with the first two
    size_t thirdIdx = 0;
    float maxArea = -1.0f;
    glm::vec3 edge = reduced[1].contactPoint - reduced[0].contactPoint;
    for (size_t i = 0; i < contacts.size(); ++i) {
        glm::vec3 cross = glm::cross(edge, contacts[i].contactPoint - reduced[0].contactPoint);
        float area = glm::length2(cross);
        if (area > maxArea) {
            maxArea = area;
            thirdIdx = i;
        }
    }
    reduced.push_back(contacts[thirdIdx]);

    // 4. Maximise quadrilateral area — pick the point farthest from the
    //    plane formed by the first three
    if (MAX_MANIFOLD_CONTACTS >= 4) {
        glm::vec3 triNormal = glm::cross(
            reduced[1].contactPoint - reduced[0].contactPoint,
            reduced[2].contactPoint - reduced[0].contactPoint
        );
        float triNormalLen = glm::length(triNormal);
        if (triNormalLen > 1e-8f) {
            triNormal /= triNormalLen;
        }

        size_t fourthIdx = 0;
        float maxDist = -1.0f;
        for (size_t i = 0; i < contacts.size(); ++i) {
            float d = std::abs(glm::dot(contacts[i].contactPoint - reduced[0].contactPoint, triNormal));
            if (d > maxDist) {
                maxDist = d;
                fourthIdx = i;
            }
        }

        // Only add if it's meaningfully off-plane; otherwise pick farthest
        // from centroid of the existing three
        if (maxDist < 1e-6f) {
            glm::vec3 centroid = (reduced[0].contactPoint + reduced[1].contactPoint + reduced[2].contactPoint) / 3.0f;
            float maxCentroidDistSq = -1.0f;
            for (size_t i = 0; i < contacts.size(); ++i) {
                float dSq = glm::length2(contacts[i].contactPoint - centroid);
                if (dSq > maxCentroidDistSq) {
                    maxCentroidDistSq = dSq;
                    fourthIdx = i;
                }
            }
        }
        reduced.push_back(contacts[fourthIdx]);
    }

    contacts = std::move(reduced);
}

// ── SphereBVHNode ────────────────────────────────────────────────────

bool SphereBVHNode::isLeaf() const {
    return left == nullptr && right == nullptr;
}

bool SphereBVHNode::checkCollision(const Collider& collider, std::vector<ContactInfo>& info) const {
    const auto* otherSphere = dynamic_cast<const SphereCollider*>(&collider);
    if (!otherSphere) return false;

    if (!spheresOverlap(sphere, *otherSphere)) {
        return false;
    }

    if (isLeaf()) {
        return sphere.checkCollision(*otherSphere, info);
    }

    bool hitLeft  = left  ? left->checkCollision(collider, info)  : false;
    bool hitRight = right ? right->checkCollision(collider, info) : false;
    return hitLeft || hitRight;
}

// ── SphereBVH ────────────────────────────────────────────────────────

SphereBVH::SphereBVH(const sauce::modeling::Mesh& mesh) {
    const auto &vertices = mesh.getVertices();
    const auto &indices = mesh.getIndices();

    if (indices.empty() || vertices.empty()) {
        return;
    }

    std::vector<TriangleInfo> triangles;
    size_t numTriangles = indices.size() / 3;
    triangles.reserve(numTriangles);

    for (size_t i = 0; i < numTriangles; ++i) {
        glm::vec3 p0 = vertices[indices[i * 3 + 0]].position;
        glm::vec3 p1 = vertices[indices[i * 3 + 1]].position;
        glm::vec3 p2 = vertices[indices[i * 3 + 2]].position;

        triangles.push_back({
            .idx = static_cast<uint32_t>(i),
            .v0 = p0, 
            .v1 = p1, 
            .v2 = p2,
            .centroid = (p0 + p1 + p2) / 3.0f 
        });
    }

    this->root = buildRecursive(triangles, 0, triangles.size());
}

bool SphereBVH::checkCollision(const Collider& collider, std::vector<ContactInfo>& info) const {
    if (!root) return false;

    // Collect all raw contacts from the tree
    std::vector<ContactInfo> rawContacts;
    bool hit = root->checkCollision(collider, rawContacts);

    if (hit && !rawContacts.empty()) {
        reduceManifold(rawContacts);
        info.insert(info.end(), rawContacts.begin(), rawContacts.end());
    }

    return hit;
}

const SphereBVHNode *SphereBVH::getRoot() const {
    return this->root.get();
}

std::unique_ptr<SphereBVHNode> SphereBVH::buildRecursive(std::vector<TriangleInfo> &triangles, size_t start, size_t end) {
    auto node = std::make_unique<SphereBVHNode>();
    size_t count = end - start;
    glm::vec3 minExt(std::numeric_limits<float>::max());
    glm::vec3 maxExt(std::numeric_limits<float>::lowest());

    for (size_t i = start; i < end; ++i) {
        const auto &t = triangles[i];
        minExt = glm::min(minExt, glm::min(t.v0, glm::min(t.v1, t.v2)));
        maxExt = glm::max(maxExt, glm::max(t.v0, glm::max(t.v1, t.v2)));
    }

    node->sphere.center = (minExt + maxExt) / 2.0f;
    float maxRadiusSq = 0.0f;
    for (size_t i = start; i < end; ++i) {
        maxRadiusSq = std::max(maxRadiusSq, glm::length2(node->sphere.center - triangles[i].v0));
        maxRadiusSq = std::max(maxRadiusSq, glm::length2(node->sphere.center - triangles[i].v1));
        maxRadiusSq = std::max(maxRadiusSq, glm::length2(node->sphere.center - triangles[i].v2));
    }
    node->sphere.radius = std::sqrt(maxRadiusSq);

    const size_t MAX_TRIANGLES_PER_LEAF = 4;
    if (count <= MAX_TRIANGLES_PER_LEAF) {
        for (size_t i = start; i < end; ++i) {
            node->triangleIndices.push_back(triangles[i].idx);
        }
        return node;
    }

    glm::vec3 centroidMin(std::numeric_limits<float>::max());
    glm::vec3 centroidMax(std::numeric_limits<float>::lowest());
    for (size_t i = start; i < end; ++i) {
        centroidMin = glm::min(centroidMin, triangles[i].centroid);
        centroidMax = glm::max(centroidMax, triangles[i].centroid);
    }

    glm::vec3 extent = centroidMax - centroidMin;
    int axis = 0; 
    if (extent.y > extent.x) axis = 1;
    if (extent.z > extent[axis]) axis = 2;

    size_t mid = start + count / 2;
    std::nth_element(triangles.begin() + start,
                     triangles.begin() + mid,
                     triangles.begin() + end,
                     [axis](const TriangleInfo &a, const TriangleInfo &b) {
                         return a.centroid[axis] < b.centroid[axis];
                     });

    node->left = buildRecursive(triangles, start, mid);
    node->right = buildRecursive(triangles, mid, end);

    return node;
}
};