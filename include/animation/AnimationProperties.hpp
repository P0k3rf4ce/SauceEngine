#ifndef ANIMATION_PROPERTIES_HPP
#define ANIMATION_PROPERTIES_HPP

#include <Eigen/Geometry>
#include <vector>
#include <memory>  
#include <limits>  

#include "modeling/ModelProperties.hpp"

namespace modeling {
    class ModelProperties;
}

namespace animation {

/**
 * Axis-Aligned Bounding Box (AABB) representation
 */
struct AABB {
    Eigen::Vector3d min;
    Eigen::Vector3d max;
    bool empty;

    AABB()
        : min(Eigen::Vector3d::Zero()), max(Eigen::Vector3d::Zero()), empty(true) {}

    AABB(const Eigen::Vector3d &min_, const Eigen::Vector3d &max_)
        : min(min_), max(max_), empty(false) {}

    // Expand this bounding box to include another
    void expand(const AABB &other) {
        if (other.empty) return;
        if (empty) { *this = other; return; }
        min = min.cwiseMin(other.min);
        max = max.cwiseMax(other.max);
        empty = false;
    }

    // Check if two bounding boxes overlap
    bool overlaps(const AABB &other) const {
        if (empty || other.empty) return false;
        return (min.x() <= other.max.x()) && (max.x() >= other.min.x()) &&
               (min.y() <= other.max.y()) && (max.y() >= other.min.y()) &&
               (min.z() <= other.max.z()) && (max.z() >= other.min.z());
    }
};

/**
 * Node for a hierarchical AABB tree.
 */
struct AABBNode {
    AABB box;
    std::unique_ptr<AABBNode> left;
    std::unique_ptr<AABBNode> right;
    std::vector<unsigned int> triangleIndices;

    bool isLeaf() const { return !left && !right; }
};

/**
 * Stores all animation related properties of an object
*/
class AnimationProperties {
private:
    Eigen::Vector3d com;
    double volume;

public:
    /**
     * Computes the center of mass and volume for the given vertices and indices
     * and stores them in the provided parameters.
     */
    static void computeCenreOfMassAndVolume(
        const std::vector<Eigen::Vector3d> &vertices, 
        const std::vector<unsigned int> &indices, 
        Eigen::Vector3d &com, 
        double &volume
    );

    AnimationProperties();
    AnimationProperties(const modeling::ModelProperties &modelProps);
    ~AnimationProperties();

    /**
     * This function is meant to load these 
     * Animation properties back into use
    */
    void load();

    /**
     * This function is meant to remove these 
     * Animation properties from use with the
     * intention that they will be used in the future.
    */
    void unload();

    /**
     * Update the Animation properties <timestep> seconds into the future
    */
    void update(double timestep);

    /**
     * Returns the model matrix for this object.
     * A model matrix places the object in the correct point in world space
    */
    Eigen::Affine3d getModelMatrix();

    Eigen::Matrix3d computeInertiaTensor(
        const std::vector<Eigen::Vector3d> &vertices,
        const std::vector<unsigned int> &indices,
        const Eigen::Vector3d &com) const;
    /**
     * Compute inverse inertia tensor (direct inversion)
     */
    static Eigen::Matrix3d computeInverseInertiaTensor(
        const Eigen::Matrix3d &inertia);

      /**
     * Compute the AABB (min/max representation) of a point set.
     */
    static AABB BoundingBoxRepresentation(const std::vector<Eigen::Vector3d> &vectors);

    /**
     * Returns true if two bounding boxes overlap.
     */
    static bool BoundingBoxOverlap(
        const std::vector<Eigen::Vector3d> &points_one,
        const std::vector<Eigen::Vector3d> &points_two
    );

    // === Hierarchical AABB tree ===

    /**
     * Recursively builds an AABB tree from a triangle mesh.
     */
    static std::unique_ptr<AABBNode> BuildAABBTree(
        const std::vector<Eigen::Vector3d> &vertices,
        const std::vector<unsigned int> &indices,
        int depth = 0
    );
    
};

}

#endif
