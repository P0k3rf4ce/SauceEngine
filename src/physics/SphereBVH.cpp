#include <physics/SphereBVH.hpp>
#include <glm/gtx/norm.hpp>

namespace physics {

// helper functions
namespace {
    struct TriangleInfo {
        uint32_t idx; 
        glm::vec3 v0, v1, v2;
        glm::vec3 centroid;
    };
    std::unique_ptr<SphereBVHNode> buildNode(std::vector<TriangleInfo> &triangles, size_t start, size_t end) {
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

        node->left = buildNode(triangles, start, mid);
        node->right = buildNode(triangles, mid, end);

        return node;
    }

    SphereCollider mergeSpheres(const SphereCollider& s1, const SphereCollider& s2) {
        float dist = glm::distance(s1.center, s2.center);

        if (dist + s1.radius <= s2.radius) {
            return s2;
        }
        if (dist + s2.radius <= s1.radius) {
            return s1;
        }

        float newRadius = (dist + s1.radius + s2.radius) / 2.0;
        
        SphereCollider res;
        res.center = glm::mix(s1.center, s2.center, (newRadius - s1.radius) / dist);
        res.radius = newRadius;
        return res;
    }

    std::unique_ptr<SphereBVHNode> buildBVH(std::vector<std::unique_ptr<SphereBVHNode>>& spheres, int start, int end) {
        int count = end - start;
        if (count <= 0) {
            return nullptr;
        }

        if (count == 1) {
            return std::move(spheres[start]);
        }
        
        glm::vec3 minCentroid(std::numeric_limits<float>::max());
        glm::vec3 maxCentroid(std::numeric_limits<float>::lowest());

        for (int i = start; i < end; ++i) {
            glm::vec3 c = spheres[i]->sphere.center;
            minCentroid = glm::min(minCentroid, c);
            maxCentroid = glm::max(maxCentroid, c);
        }

        glm::vec3 extent = maxCentroid - minCentroid;
        int axis = 0;
        if (extent.y > extent.x) {
            axis = 1;
        } else if (extent.z > extent[axis]) {
            axis = 2;
        }

        int mid = start + count / 2;
        std::nth_element(
            spheres.begin() + start,
            spheres.begin() + mid,
            spheres.begin() + end,
            [axis](const std::unique_ptr<SphereBVHNode>& a, const std::unique_ptr<SphereBVHNode>& b) {
                return a->sphere.center[axis] < b->sphere.center[axis];
            }
        );

        auto node = std::make_unique<SphereBVHNode>();
        node->left = buildBVH(spheres, start, mid);
        node->right = buildBVH(spheres, mid, end);
        node->sphere = mergeSpheres(node->left->sphere, node->right->sphere);

        return node;
    }
}

// SphereBVHNode
bool SphereBVHNode::checkCollision(const Collider& collider, std::vector<ContactInfo>& info) const {
    if (!this->sphere.checkCollision(collider, info)) {
        return false;
    }

    return this->left->sphere.checkCollision(collider, info) 
        || this->right->sphere.checkCollision(collider, info);
}

std::unique_ptr<SphereBVHNode> SphereBVHNode::fromMesh(sauce::modeling::Mesh& mesh) {
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

    return buildNode(triangles, 0, triangles.size());
}

bool SphereBVHNode::isLeaf() const {
    return left == nullptr && right == nullptr;
}



// SphereBVH
SphereBVH SphereBVH::fromScene(const sauce::Scene& scene) {
    
    // make BVH for every mesh
    std::vector<std::unique_ptr<SphereBVHNode>> spheres;
    auto &entities = scene.getEntities();
    for (auto entity: entities) {
        auto mesh_renderer = entity.getComponent<sauce::MeshRendererComponent>();
        if (mesh_renderer != nullptr) {
            spheres.push_back(SphereBVHNode::fromMesh(*mesh_renderer->getMesh()));
        }
    }
    
    // Combine every BVH into a large one or the scene
    return SphereBVH(buildBVH(spheres, 0, entities.size()));
}

bool SphereBVH::checkCollision(const Collider& collider, std::vector<ContactInfo>& info) const {
    return this->getRoot()->sphere.checkCollision(collider, info);
}

SphereBVHNode *SphereBVH::getRoot() const {
    return this->root.get();
}


}; 
