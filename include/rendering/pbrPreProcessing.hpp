#ifndef PBR_PREPROCESSING_HPP
#define PBR_PREPROCESSING_HPP

#include <glad/glad.h>
#include <Eigen/Geometry>
#include <string>

#define PI 3.14159265358979323846f


typedef unsigned int uint;

namespace rendering {


/**
 * given an HDR environment map, generate a cubemap
 * @param hdrEnvMap path to the HDR environment map
 * @return the OpenGL ID of the cubemap texture
 */
GLuint genEnvCubemap(
  const std::string hdrEnvMap
);

/**
 * given an environment cubemap, generate an irradiance map
 * @param envCubemap the OpenGL ID of the environment cubemap texture
 * @param captureFBO framebuffer object used for rendering
 * @param captureRBO renderbuffer object used for depth testing
 * @return the OpenGL ID of the irradiance cubemap texture
 */
GLuint genIrradianceMap(
  const GLuint& envCubemap,
  const GLuint& captureFBO,
  const GLuint& captureRBO
);

 /**
  * given an environment cubemap, generate a pre-filtered environment map
  * @param envCubemap the OpenGL ID of the environment cubemap texture
  * @param hdrEnvMap path to the HDR environment map
  * @param captureFBO framebuffer object used for rendering
  * @param captureRBO renderbuffer object used for depth testing
  * @param captureViews view matrices for cubemap face rendering
  * @param captureProj projection matrix for cubemap rendering
  * @return the OpenGL ID of the pre-filtered cubemap texture
  */
GLuint genPrefilterMap(
  const GLuint& envCubemap,
  const std::string hdrEnvMap,
  const GLuint& captureFBO,
  const GLuint& captureRBO,
  const std::array<Eigen::Affine3d, 6>& captureViews,
  const Eigen::Affine3d& captureProj
);

/**
 * given an environment cubemap, generate a BRDF lookup texture
 * @param envCubemap the OpenGL ID of the environment cubemap texture
 * @param captureFBO framebuffer object used for rendering
 * @param captureRBO renderbuffer object used for depth testing
 * @param captureViews view matrices for cubemap face rendering
 * @param captureProj projection matrix for cubemap rendering
 * @return the OpenGL ID of the BRDF lookup texture
 */
GLuint genBRDFLUT(
  const GLuint& envCubemap,
  const GLuint& captureFBO,
  const GLuint& captureRBO,
  const std::array<Eigen::Affine3d, 6>& captureViews,
  const Eigen::Affine3d& captureProj
);


}

#endif