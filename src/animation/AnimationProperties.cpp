#include "animation/AnimationProperties.hpp"

using namespace animation;

AnimationProperties::AnimationProperties(const modeling::ModelProperties &modelProps) {

}

AnimationProperties::~AnimationProperties() {

}

/**
 * This function is meant to load these Animation properties back into use
*/
void AnimationProperties::load() {

}

/**
 * This function is meant to remove these Animation properties from use with the
 * intention that they will be used in the future.
*/
void AnimationProperties::unload() {

}

/**
 * Update the Animation properties <timestep> seconds into the future
*/
void AnimationProperties::update(double timestep) {

}

/**
 * Returns the model matrix for this object.
 * A model matrix places the object in the correct point in world space
*/
Eigen::Affine3d getModelMatrix() {
    return Eigen::Affine3d::Identity();
}
