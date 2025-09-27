#pragma once
#include <glad/glad.h>

/**
 * @brief Initialize bloom effect resources.
 *
 * @param width The width of the render target.
 * @param height The height of the render target.
 * @return true if successful, false otherwise.
 */
bool initBloom(int width, int height);

/**
 * @brief Apply bloom effect to the scene.
 *        NOTE: You need to call initBloom(width, height) first AND
 *              ensure MRT is used in fragment shader before this.
 *
 * @param sceneTex The original scene texture.
 * @param brightTex The bright parts texture.
 * @param iterations The number of iterations for the bloom effect.
 * @param exposure The exposure adjustment.
 * @return GLuint The ID of the bloom texture, or 0 on failure.
 */
GLuint applyBloom(
    GLuint sceneTex,
    GLuint brightTex,
    int iterations,
    float exposure);

/**
 * @brief Apply bloom effect with custom kernel.
 *        NOTE: You need to call initBloom(width, height) first AND
 *              ensure MRT is used in fragment shader before this.
 * @param sceneTex The original scene texture.
 * @param brightTex The bright parts texture.
 * @param kernelRadius The radius of the Gaussian kernel (MUST be odd).
 * @param sigma The standard deviation for the Gaussian kernel (use -1 for auto).
 * @param exposure The exposure adjustment.
 * @return GLuint The ID of the bloom texture, or 0 on failure.
 */
GLuint applyBloomWithKernel(GLuint sceneTex,
                            GLuint brightTex,
                            int kernelRadius,
                            float sigma,
                            float exposure);

/**
 * @brief Destroy bloom effect resources.
 *
 */
void destroyBloomResources();
