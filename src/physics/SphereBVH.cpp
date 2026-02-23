#include <algorithm>
#include <app/modeling/Mesh.hpp>
#include <physics/Collider.hpp>
#include <physics/SphereBVH.hpp>
#include <vector>
#include <glm/gtx/norm.hpp>
namespace physics {

bool SphereBVHNode::isLeaf() const {
    return left == nullptr && right == nullptr;
}

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
    return this->getRoot()->sphere.checkCollision(collider, info);
}

bool SphereBVHNode::checkCollision(const Collider& collider, std::vector<ContactInfo>& info) const {
    if (this->sphere.checkCollision(collider, info)) {
        if (this->isLeaf()) {
            // TODO collision detection between triangles 
            return;
        }
        return this->left->sphere.checkCollision(collider, info) 
            || this->right->sphere.checkCollision(collider, info);
    }
    return false;
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
