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

// ---------------------------
// Everything below is for bouding boxes
// ---------------------------

AABB AnimationProperties::BoundingBoxRepresentation(const std::vector<Eigen::Vector3d> &vectors) {
    if (vectors.empty()) return AABB();

    Eigen::Vector3d min = vectors[0];
    Eigen::Vector3d max = vectors[0];

    for (const auto &v : vectors) {
        min = min.cwiseMin(v);
        max = max.cwiseMax(v);
    }

    return AABB(min, max);
}

bool AnimationProperties::BoundingBoxOverlap(const std::vector<Eigen::Vector3d> &points_one,
                                             const std::vector<Eigen::Vector3d> &points_two) 
{
    AABB bbox1 = BoundingBoxRepresentation(points_one);
    AABB bbox2 = BoundingBoxRepresentation(points_two);
    return bbox1.overlaps(bbox2);
}

std::unique_ptr<AABBNode> AnimationProperties::BuildAABBTree(
    const std::vector<Eigen::Vector3d> &vertices,
    const std::vector<unsigned int> &indices,
    int depth)
{
    auto node = std::make_unique<AABBNode>();

    std::vector<Eigen::Vector3d> triPoints;
    triPoints.reserve(indices.size());
    for (auto i : indices)
        triPoints.push_back(vertices[i]);

    node->box = BoundingBoxRepresentation(triPoints);

    if (indices.size() <= 6 || depth > 16) {
        node->triangleIndices = indices;
        return node;
    }

    Eigen::Vector3d extents = node->box.max - node->box.min;
    int axis;
    extents.maxCoeff(&axis);

    std::vector<std::pair<double, unsigned int>> centroidPairs;
    for (size_t i = 0; i < indices.size(); i += 3) {
        Eigen::Vector3d centroid = (
            vertices[indices[i]] +
            vertices[indices[i+1]] +
            vertices[indices[i+2]]) / 3.0;
        centroidPairs.push_back({centroid[axis], static_cast<unsigned int>(i)});
    }

    std::sort(centroidPairs.begin(), centroidPairs.end(),
              [](auto &a, auto &b){ return a.first < b.first; });

    size_t mid = centroidPairs.size() / 2;
    std::vector<unsigned int> leftIndices, rightIndices;
    for (size_t i = 0; i < centroidPairs.size(); ++i) {
        for (int j = 0; j < 3; ++j) {
            unsigned int idx = indices[centroidPairs[i].second + j];
            if (i < mid)
                leftIndices.push_back(idx);
            else
                rightIndices.push_back(idx);
        }
    }

    node->left = BuildAABBTree(vertices, leftIndices, depth + 1);
    node->right = BuildAABBTree(vertices, rightIndices, depth + 1);

    return node;
}