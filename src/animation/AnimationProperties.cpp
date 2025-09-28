#include "animation/AnimationProperties.hpp"

using namespace animation;

void AnimationProperties::computeCenreOfMassAndVolume(
    const std::vector<Eigen::Vector3d> &vertices, 
    const std::vector<unsigned int> &indices,
    Eigen::Vector3d &com, 
    double &volume
) {
    auto comX24Xvolume = Eigen::Vector3d(0,0,0);
    double volumeX6 = 0;
    for(auto indices_iter = indices.begin(); indices_iter != indices.end(); indices_iter += 3) {
        // A = column matrix [v0 v1 v2]
        Eigen::Matrix3d A;
        A.col(0) = vertices[indices_iter[0]];
        A.col(1) = vertices[indices_iter[1]];
        A.col(2) = vertices[indices_iter[2]];

        double curVolumeX6 = A.determinant();
        comX24Xvolume += curVolumeX6 * (A.col(0) + A.col(1) + A.col(2));
        volumeX6 += curVolumeX6;
    }
    com = comX24Xvolume / (volumeX6 ? 4.0 * volumeX6 : 1.0);
    volume = std::abs(volumeX6 / 6.0);
}

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
