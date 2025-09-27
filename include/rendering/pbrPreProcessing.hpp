#ifndef PBR_PREPROCESSING_HPP
#define PBR_PREPROCESSING_HPP

#include <Eigen/Core>
#include <string>

typedef unsigned int uint;

namespace rendering {

/**
 * contains some helper functions for pre-processing environment maps for PBR
 *    as well as pbr shader initialization
 */


/**
 * given an HDR environment map, generate a cubemap
 * 
 * returns the OpenGL ID of the cubemap
 */
uint genEnvCubemap(const std::string hdrEnvMap);

/**
 * given an environment cubemap, generate an irradiance map
 * 
 * returns the OpenGL ID of the irradiance map
 */
uint genIrradianceMap(const uint& envCubemap, const uint& captureFBO, const uint& captureRBO, const std::array<Eigen::Matrix4i, 6>& captureViews, const Eigen::Matrix4i& captureProj);

 /**
  * given an environment cubemap, generate a pre-filtered environment map
  * 
  * returns the OpenGL ID of the pre-filtered environment map
  */
uint genPrefilterMap(const uint& envCubemap, const uint& captureFBO, const uint& captureRBO, const std::array<Eigen::Matrix4i, 6>& captureViews, const Eigen::Matrix4i& captureProj);

/**
 * given an environment cubemap, generate a BRDF lookup texture
 * 
 * returns the OpenGL ID of the BRDF LUT
 */
uint genBRDFLUT(const uint& envCubemap, const uint& captureFBO, const uint& captureRBO, const std::array<Eigen::Matrix4i, 6>& captureViews, const Eigen::Matrix4i& captureProj);


}

#endif