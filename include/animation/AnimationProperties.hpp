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
public:
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
    Eigen::Affine3d getModelMatrix() const;
};

}

#endif