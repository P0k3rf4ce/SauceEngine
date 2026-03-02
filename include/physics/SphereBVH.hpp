#include <vector>
#include <algorithm>
#include <app/modeling/Mesh.hpp>
#include <app/Scene.hpp>
#include <physics/Collider.hpp>
#include <physics/SphereCollider.hpp>

namespace physics {

struct SphereBVHNode: public Collider {
    SphereCollider sphere;
    std::unique_ptr<SphereBVHNode> left;
    std::unique_ptr<SphereBVHNode> right;
    
    std::vector<uint32_t> triangleIndices;

    virtual bool checkCollision(const Collider& collider, std::vector<ContactInfo>& info) const = 0;

    bool isLeaf() const;
};

class SphereBVH: public Collider {
public:
    SphereBVH(const sauce::modeling::Mesh& mesh);

    virtual bool checkCollision(const Collider& collider, std::vector<ContactInfo>& info) const = 0;

    const SphereBVHNode* getRoot() const;

private:
    std::unique_ptr<SphereBVHNode> root;

    struct TriangleInfo {
        uint32_t idx; 
        glm::vec3 v0, v1, v2;
        glm::vec3 centroid;
    };

    void buildFromMesh(const sauce::modeling::Mesh& mesh);

    std::unique_ptr<SphereBVHNode> buildRecursive(std::vector<TriangleInfo>& triangles, size_t start, size_t end);
};
}
