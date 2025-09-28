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
    std::vector<Eigen::Vector3d> vertices = modelProps.vertices;
    std::vector<Eigen::Vector3i> tris = modelProps.triangles;
    Eigen::Vector3d com = modelProps.centerOfMass;

    inertiaTensor_ = computeInertiaTensor(vertices, tris, com);
    inverseInertiaTensor_ = inertiaTensor_.inverse();
    currentTime_ = 0.0;
    loaded_ = false;
}

Eigen::Matrix3d AnimationProperties::computeInertiaTensor(
    const std::vector<Eigen::Vector3d> &vertices,
    const std::vector<Eigen::Vector3i> &tris,
    const Eigen::Vector3d &com) const
{
    double volume = 0.0;
    Eigen::Vector3d diag(0,0,0);
    Eigen::Vector3d offd(0,0,0);

    for (const auto &tri : tris) {
        Eigen::Matrix3d A;
        A.col(0) = vertices[tri[0]] - com;
        A.col(1) = vertices[tri[1]] - com;
        A.col(2) = vertices[tri[2]] - com;

        double d = determinantFromCols(A.col(0), A.col(1), A.col(2));
        volume += d;

        for (int j = 0; j < 3; j++) {
            int j1 = (j + 1) % 3;
            int j2 = (j + 2) % 3;

            diag[j] += (
                A(0,j)*A(1,j) + A(1,j)*A(2,j) + A(2,j)*A(0,j) +
                A(0,j)*A(0,j) + A(1,j)*A(1,j) + A(2,j)*A(2,j)
            ) * d;

            offd[j] += (
                A(0,j1)*A(1,j2) + A(1,j1)*A(2,j2) + A(2,j1)*A(0,j2) +
                A(0,j1)*A(2,j2) + A(1,j1)*A(0,j2) + A(2,j1)*A(1,j2) +
                2*A(0,j1)*A(0,j2) + 2*A(1,j1)*A(1,j2) + 2*A(2,j1)*A(2,j2)
            ) * d;
        }
    }

    diag /= volume * (60.0 / 6.0);
    offd /= volume * (120.0 / 6.0);

    Eigen::Matrix3d tensor;
    tensor << diag.y() + diag.z(), -offd.z(),          -offd.y(),
              -offd.z(),           diag.x() + diag.z(), -offd.x(),
              -offd.y(),           -offd.x(),           diag.x() + diag.y();

    return tensor;
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

// From https://www.melax.com/volint.html
// float3x3 Inertia(const float3 *vertices, const int3 *tris, const int count, const float3& com)  
// {
// 	// count is the number of triangles (tris) 
// 	// The moments are calculated based on the center of rotation com which you should calculate first
// 	// assume mass==1.0  you can multiply by mass later.
// 	// for improved accuracy the next 3 variables, the determinant d, and its calculation should be changed to double
// 	float  volume=0;                          // technically this variable accumulates the volume times 6
// 	float3 diag(0,0,0);                       // accumulate matrix main diagonal integrals [x*x, y*y, z*z]
// 	float3 offd(0,0,0);                       // accumulate matrix off-diagonal  integrals [y*z, x*z, x*y]
// 	for(int i=0; i < count; i++)  // for each triangle
// 	{
// 		float3x3 A(vertices[tris[i][0]]-com,vertices[tris[i][1]]-com,vertices[tris[i][2]]-com);  // matrix trick for volume calc by taking determinant
// 		float    d = Determinant(A);  // vol of tiny parallelapiped= d * dr * ds * dt (the 3 partials of my tetral triple integral equasion)
// 		volume +=d;                   // add vol of current tetra (note it could be negative - that's ok we need that sometimes)
// 		for(int j=0;j < 3;j++)
// 		{
// 			int j1=(j+1)%3;   
// 			int j2=(j+2)%3;   
// 			diag[j] += (A[0][j]*A[1][j] + A[1][j]*A[2][j] + A[2][j]*A[0][j] + 
// 			            A[0][j]*A[0][j] + A[1][j]*A[1][j] + A[2][j]*A[2][j]  ) *d; // divide by 60.0f later;
// 			offd[j] += (A[0][j1]*A[1][j2]  + A[1][j1]*A[2][j2]  + A[2][j1]*A[0][j2]  +
// 			            A[0][j1]*A[2][j2]  + A[1][j1]*A[0][j2]  + A[2][j1]*A[1][j2]  +
// 			            A[0][j1]*A[0][j2]*2+ A[1][j1]*A[1][j2]*2+ A[2][j1]*A[2][j2]*2 ) *d; // divide by 120.0f later
// 		}
// 	}
// 	diag /= volume*(60.0f /6.0f);  // divide by total volume (vol/6) since density=1/volume
// 	offd /= volume*(120.0f/6.0f);
// 	return float3x3(diag.y+diag.z  , -offd.z      , -offd.y,
// 				-offd.z        , diag.x+diag.z, -offd.x,
// 				-offd.y        , -offd.x      , diag.x+diag.y );
// }
