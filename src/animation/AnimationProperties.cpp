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

AnimationProperties::AnimationProperties() {

}

AnimationProperties::AnimationProperties(const modeling::ModelProperties &modelProps) {

}


Eigen::Matrix3d AnimationProperties::computeInertiaTensor(
    const std::vector<Eigen::Vector3d> &vertices,
    const std::vector<unsigned int> &indices,
    const Eigen::Vector3d &com) const 
{
    Eigen::Matrix3d inertia = Eigen::Matrix3d::Zero();

    double totalVolume = 0.0;

    for (size_t i = 0; i < indices.size(); i += 3) {
        const Eigen::Vector3d &v0 = vertices[indices[i]];
        const Eigen::Vector3d &v1 = vertices[indices[i + 1]];
        const Eigen::Vector3d &v2 = vertices[indices[i + 2]];

        Eigen::Vector3d r0 = v0 - com;
        Eigen::Vector3d r1 = v1 - com;
        Eigen::Vector3d r2 = v2 - com;

        double vol = r0.dot(r1.cross(r2)) / 6.0;
        totalVolume += vol;

        Eigen::Matrix3d C = (r0 * r0.transpose() +
                             r1 * r1.transpose() +
                             r2 * r2.transpose() +
                             r0 * r1.transpose() +
                             r1 * r2.transpose() +
                             r2 * r0.transpose());

        inertia += vol * C / 10.0; 
    }

    inertia = 0.5 * (inertia + inertia.transpose());

    Eigen::Matrix3d inertiaTensor = Eigen::Matrix3d::Zero();
    inertiaTensor(0,0) = inertia(1,1) + inertia(2,2);
    inertiaTensor(1,1) = inertia(0,0) + inertia(2,2);
    inertiaTensor(2,2) = inertia(0,0) + inertia(1,1);
    inertiaTensor(0,1) = inertiaTensor(1,0) = -inertia(0,1);
    inertiaTensor(1,2) = inertiaTensor(2,1) = -inertia(1,2);
    inertiaTensor(0,2) = inertiaTensor(2,0) = -inertia(0,2);

    return inertiaTensor;
}

Eigen::Matrix3d AnimationProperties::computeInverseInertiaTensor(
    const Eigen::Matrix3d &inertia)
{    return inertia.inverse();
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
Eigen::Affine3d AnimationProperties::getModelMatrix() {
    return Eigen::Affine3d::Identity();
}

void AnimationProperties::computeBoundingBoxHierarchy(
    const std::vector<Eigen::Vector3d> &vertices,
    const std::vector<unsigned int> &indices
) {
    boundingBoxes.clear();

    for (size_t i = 0; i < indices.size(); i += 3) {
        Eigen::AlignedBox3d box;
        box.setEmpty();
        box.extend(vertices[indices[i]]);
        box.extend(vertices[indices[i + 1]]);
        box.extend(vertices[indices[i + 2]]);
        boundingBoxes.push_back(box);
    }
}

bool AnimationProperties::boxesOverlap(
    const Eigen::AlignedBox3d &a,
    const Eigen::AlignedBox3d &b
) const {
    return a.intersects(b);
}