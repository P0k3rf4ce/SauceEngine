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
GLuint genEnvCubemap(const std::string hdrEnvMap);

/**
 * given an environment cubemap, generate an irradiance map
 * @param envCubemap the OpenGL ID of the environment cubemap texture
 * @return the OpenGL ID of the irradiance cubemap texture
 */
GLuint genIrradianceMap(const GLuint& envCubemap);

 /**
  * given an environment cubemap, generate a pre-filtered environment map
  * @param envCubemap the OpenGL ID of the environment cubemap texture
  * @return the OpenGL ID of the pre-filtered cubemap texture
  */
GLuint genPrefilterMap(const GLuint& envCubemap);

/**
 * given an environment cubemap, generate a BRDF lookup texture
 * @param envCubemap the OpenGL ID of the environment cubemap texture
 * @return the OpenGL ID of the BRDF lookup texture
 */
GLuint genBRDFLUT(const GLuint& envCubemap);


}

#endif