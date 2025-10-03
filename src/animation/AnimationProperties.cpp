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
    const std::vector<Eigen::Vector3i> &tris,
    const Eigen::Vector3d &com) {
    double volume = 0.0;
    Eigen::Vector3d diag = Eigen::Vector3d::Zero();
    Eigen::Vector3d offd = Eigen::Vector3d::Zero();

    for (int i = 0; i < tris.size(); i++) {
        Eigen::Matrix3d A;
        A.col(0) = vertices[tris[i][0]] - com;
        A.col(1) = vertices[tris[i][1]] - com;
        A.col(2) = vertices[tris[i][2]] - com;
        double d = A.determinant();
        volume += d;

        for (int j = 0; j < 3; j++) {
            int j1 = (j + 1) % 3;
            int j2 = (j + 2) % 3;
            diag[j] += (A(0,j)*A(1,j) + A(1,j)*A(2,j) + A(2,j)*A(0,j) +
                        A(0,j)*A(0,j) + A(1,j)*A(1,j) + A(2,j)*A(2,j)) * d;
            offd[j] += (A(0,j1)*A(1,j2) + A(1,j1)*A(2,j2) + A(2,j1)*A(0,j2) +
                        A(0,j1)*A(2,j2) + A(1,j1)*A(0,j2) + A(2,j1)*A(1,j2) +
                        2*A(0,j1)*A(0,j2) + 2*A(1,j1)*A(1,j2) + 2*A(2,j1)*A(2,j2)) * d;
        }
    }

    diag /= volume * (60.0 / 6.0);
    offd /= volume * (120.0 / 6.0);

    Eigen::Matrix3d inertia;
    inertia << diag.y() + diag.z(),   -offd.z(),            -offd.y(),
               -offd.z(),             diag.x() + diag.z(),  -offd.x(),
               -offd.y(),             -offd.x(),            diag.x() + diag.y();

    return inertia; 
}

Eigen::Matrix3d AnimationProperties::computeInverseInertiaTensor(
    const Eigen::Matrix3d &inertia)
{
    // Direct inversion (fast, but less safe if the matrix is nearly singular)
    return inertia.inverse();
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
