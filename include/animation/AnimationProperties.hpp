#ifndef ANIMATION_PROPERTIES_HPP
#define ANIMATION_PROPERTIES_HPP

#include <Eigen/Geometry>

#include "modeling/ModelProperties.hpp"

namespace modeling {
    class ModelProperties;
}

namespace animation {

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

    static Eigen::Matrix3d computeInertiaTensor(
        const std::vector<Eigen::Vector3d> &vertices,
        const std::vector<Eigen::Vector3i> &tris,
        const Eigen::Vector3d &com);

    /**
     * Compute inverse inertia tensor (direct inversion)
     */
    static Eigen::Matrix3d computeInverseInertiaTensor(
        const Eigen::Matrix3d &inertia);
};

}

#endif
