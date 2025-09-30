#ifndef BRDF_HPP
#define BRDF_HPP

#include <Eigen/Dense>

namespace rendering {

/**
 * Generates a 2D LUT from the BRDF equations used for IBL.
 * @param envCubemap The environment cubemap.
 * @param captureFBO The framebuffer object to use for capturing.
 * @param captureRBO The renderbuffer object to use for capturing.
 * @param captureViews The capture projection-view matrices.
 * @param captureProj The capture projection matrix.
 * @return The ID of the generated BRDF LUT texture.
 */
unsigned int genBRDFLUT(
    unsigned int envCubemap,
    unsigned int captureFBO,
    unsigned int captureRBO,
    const Eigen::Matrix4f* captureViews,
    const Eigen::Matrix4f& captureProj
);

}

#endif
