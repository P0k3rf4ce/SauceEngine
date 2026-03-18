#pragma once

#include <algorithm>
#include <app/Entity.hpp>
#include <app/Scene.hpp>
#include <app/components/MeshRendererComponent.hpp>
#include <app/modeling/Mesh.hpp>
#include <physics/Collider.hpp>
#include <physics/SphereCollider.hpp>
#include <vector>

namespace physics {

    struct SphereBVHNode : public Collider {
        SphereCollider sphere;
        std::unique_ptr<SphereBVHNode> left;
        std::unique_ptr<SphereBVHNode> right;

        std::vector<uint32_t> triangleIndices;

        bool checkCollision(const Collider& collider, std::vector<ContactInfo>& info) const;

        static std::unique_ptr<SphereBVHNode> fromMesh(sauce::modeling::Mesh& mesh);
        bool isLeaf() const;
    };

    class SphereBVH : public Collider {
      public:
        static SphereBVH fromScene(const sauce::Scene& scene);

        bool checkCollision(const Collider& collider, std::vector<ContactInfo>& info) const;

        SphereBVHNode* getRoot() const;

      private:
        std::unique_ptr<SphereBVHNode> root;

        SphereBVH(std::unique_ptr<SphereBVHNode> root) : root(std::move(root)) {
        }
    };
} // namespace physics
