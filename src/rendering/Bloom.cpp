#include <string>
#include <unordered_map>
#include <vector>
#include <cmath>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "utils/Shader.hpp"
#include "rendering/Quad.hpp"
#include "rendering/Bloom.hpp"

namespace
{
    // Shaders
    // uniforms: image, horizontal, kernelSize, weights[], texelSize
    Shader blurShader;
    // uniforms: scene, bloomBlur, bloom, exposure
    Shader combineShader;
    bool shadersReady = false;

    // Fixed-size resources -- set by initBloom(width, height)
    GLuint pingFbo[2] = {0, 0};
    GLuint pingTex[2] = {0, 0};
    GLuint outFbo = 0;
    GLuint outTex = 0;
    int widthFixed = 0;
    int heightFixed = 0;

    /**
     * @brief Allocates a color texture with RGBA16F format.
     *
     * @param texturePtr Pointer to the texture ID.
     * @param width Width of the texture.
     * @param height Height of the texture.
     * @return true if allocation was successful, false otherwise.
     */
    bool allocateColorTextureRGBA16F(GLuint *texturePtr, int width, int height)
    {
        if (!texturePtr)
            return false;
        if (*texturePtr == 0)
            glGenTextures(1, texturePtr);
        glBindTexture(GL_TEXTURE_2D, *texturePtr);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        return true;
    }

    /**
     * @brief Create a Ping Pong framebuffer object and associated textures.
     *
     * @return true if creation was successful, false otherwise.
     */
    bool createPingPong()
    {
        for (int i = 0; i < 2; ++i)
        {
            if (pingFbo[i] == 0)
                glGenFramebuffers(1, &pingFbo[i]);
            glBindFramebuffer(GL_FRAMEBUFFER, pingFbo[i]);

            if (!allocateColorTextureRGBA16F(&pingTex[i], widthFixed, heightFixed))
            {
                LOG_ERROR(("Bloom: failed to allocate ping-pong texture " + std::to_string(i)).c_str());
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                return false;
            }
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingTex[i], 0);
            glDrawBuffer(GL_COLOR_ATTACHMENT0);

            GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE)
            {
                LOG_ERROR(("Bloom: ping-pong FBO " + std::to_string(i) + " incomplete (0x" +
                           std::to_string(static_cast<unsigned int>(status)) + ")")
                              .c_str());
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                return false;
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return true;
    }

    /**
     * @brief Create the output framebuffer object and associated texture.
     *
     * @return true if creation was successful, false otherwise.
     */
    bool createOutputTarget()
    {
        if (outFbo == 0)
            glGenFramebuffers(1, &outFbo);
        glBindFramebuffer(GL_FRAMEBUFFER, outFbo);

        if (!allocateColorTextureRGBA16F(&outTex, widthFixed, heightFixed))
        {
            LOG_ERROR("Bloom: failed to allocate output texture");
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return false;
        }
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outTex, 0);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            LOG_ERROR(("Bloom: output FBO incomplete (0x" + std::to_string(static_cast<unsigned int>(status)) + ")").c_str());
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return true;
    }

    /**
     * @brief Ensure that all required shaders are loaded.
     *
     * @return true if shaders are ready, false otherwise.
     */
    bool ensureShadersLoaded()
    {
        if (shadersReady)
            return true;

        // Swap Joey's fixed blur for our dynamic kernel blur shader.
        const std::unordered_map<SHADER_TYPE, std::string> blurFiles = {
            {VERTEX, "shaders/bloom/bloom.vs"},
            {FRAGMENT, "shaders/bloom/blur.fs"}};
        const std::unordered_map<SHADER_TYPE, std::string> combineFiles = {
            {VERTEX, "shaders/bloom/bloom.vs"},
            {FRAGMENT, "shaders/bloom/bloom_blend.fs"}};

        bool ok = blurShader.loadFromFiles(blurFiles) && combineShader.loadFromFiles(combineFiles);
        if (!ok)
        {
            LOG_ERROR("Bloom: failed to load blur/combine shaders. Check asset paths.");
            return false;
        }
        shadersReady = true;

        blurShader.bind();
        blurShader.setUniform("image", 0);
        blurShader.unbind();

        combineShader.bind();
        combineShader.setUniform("scene", 0);
        combineShader.setUniform("bloomBlur", 1);
        combineShader.unbind();

        return true;
    }

    /**
     * @brief Build a normalized 1D Gaussian kernel.
     *
     * @param kernelRadius Radius of the kernel (half-size).
     * @param sigma Standard deviation of the Gaussian.
     * @param weightsOut Output vector to store the kernel weights.
     * @return true if successful, false otherwise.
     */
    bool buildGaussianKernel(int kernelRadius, float sigma, std::vector<float> *weightsOut)
    {
        if (!weightsOut)
            return false;
        weightsOut->clear();

        if (kernelRadius < 0)
            kernelRadius = 0;

        // Clamp to max radius supported by shader array size.
        if (kernelRadius > 63)
            kernelRadius = 63;

        // Choose standard deviation if not provided.
        if (sigma <= 0.0f)
            sigma = kernelRadius * 0.5f + 0.5f;

        const float twoSigmaSq = 2.0f * sigma * sigma;

        // Compute unnormalized w[i] for i in [0..radius]
        weightsOut->reserve(static_cast<size_t>(kernelRadius) + 1);
        for (int i = 0; i <= kernelRadius; ++i)
        {
            float w = std::exp(-(i * i) / twoSigmaSq);
            weightsOut->push_back(w);
        }

        // Normalize so that full symmetric kernel sums to 1:
        // sum = w[0] + 2 * sum_{i=1..radius} w[i]
        float sum = (*weightsOut)[0];
        for (int i = 1; i <= kernelRadius; ++i)
            sum += 2.0f * (*weightsOut)[i];
        if (sum <= 0.0f)
            return false;
        for (float &v : *weightsOut)
            v /= sum;
        return true;
    }

    /**
     * @brief Upload Gaussian kernel parameters to the shader.
     *
     * @param shader The shader to upload to.
     * @param kernelRadius The radius of the kernel.
     * @param weights The kernel weights.
     * @return true if successful, false otherwise.
     */
    bool uploadKernelToShader(Shader *shader, int kernelRadius, std::vector<float> *weights)
    {
        if (!shader || !weights)
            return false;

        int kernelSize = kernelRadius * 2 + 1;
        shader->setUniform("kernelSize", kernelSize);

        // weights array is declared as weights[64]; we set 0..radius
        for (int i = 0; i <= kernelRadius; ++i)
        {
            shader->setUniform("weights[" + std::to_string(i) + "]", (*weights)[static_cast<size_t>(i)]);
        }

        return true;
    }

    /**
     * @brief Run a separable Gaussian blur on the given texture.
     *        Performs two passes: horizontal and vertical.
     *
     * @param sourceTex The source texture to blur.
     * @param kernelRadius The radius of the Gaussian kernel.
     * @param sigma The standard deviation of the Gaussian.
     * @return GLuint The ID of the blurred texture, or 0 on failure.
     */
    GLuint runSeparableGaussianBlur(GLuint sourceTex, int kernelRadius, float sigma)
    {
        if (!shadersReady)
            return 0;
        if (sourceTex == 0)
        {
            LOG_ERROR("Bloom: runSeparableGaussianBlur sourceTex == 0");
            return 0;
        }
        if (kernelRadius < 0)
            kernelRadius = 0;
        if (kernelRadius > 63)
            kernelRadius = 63;

        blurShader.bind();

        std::vector<float> weights;
        if (!buildGaussianKernel(kernelRadius, sigma, &weights))
        {
            LOG_ERROR("Bloom: buildGaussianKernel failed.");
            blurShader.unbind();
            return 0;
        }
        if (!uploadKernelToShader(&blurShader, kernelRadius, &weights))
        {
            LOG_ERROR("Bloom: uploadKernelToShader failed.");
            blurShader.unbind();
            return 0;
        }

        glDisable(GL_DEPTH_TEST);

        // Pass 1: Horizontal, sourceTex -> pingTex[1]
        glBindFramebuffer(GL_FRAMEBUFFER, pingFbo[1]);
        glViewport(0, 0, widthFixed, heightFixed);

        blurShader.setUniform("horizontal", true);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sourceTex);
        blurShader.setUniform("image", 0);

        renderQuad();

        // Pass 2: Vertical, pingTex[1] -> pingTex[0]
        glBindFramebuffer(GL_FRAMEBUFFER, pingFbo[0]);
        glViewport(0, 0, widthFixed, heightFixed);

        blurShader.setUniform("horizontal", false);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pingTex[1]);
        blurShader.setUniform("image", 0);

        renderQuad();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        blurShader.unbind();

        // Result is now in pingTex[0]
        return pingTex[0];
    }

    /**
     * @brief Composite (additively blend) the scene and bloom textures.
     *
     * @param sceneTex The scene texture.
     * @param bloomTex The bloom (blurred) texture.
     * @param exposure The exposure adjustment.
     * @return true if successful, false otherwise.
     */
    bool compositeSceneAndBloom(GLuint sceneTex, GLuint bloomTex, float exposure)
    {
        combineShader.bind();

        glBindFramebuffer(GL_FRAMEBUFFER, outFbo);
        glViewport(0, 0, widthFixed, heightFixed);
        glDisable(GL_DEPTH_TEST);

        combineShader.setUniform("scene", 0);
        combineShader.setUniform("bloomBlur", 1);
        combineShader.setUniform("exposure", exposure);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sceneTex);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, bloomTex);

        renderQuad();

        combineShader.unbind();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return true;
    }
} // anonymous namespace

bool initBloom(int width, int height)
{
    if (width <= 0 || height <= 0)
    {
        std::string errMsg = "initBloom: invalid size " + std::to_string(width) + "x" + std::to_string(height);
        LOG_ERROR(errMsg.c_str());
        return false;
    }
    widthFixed = width;
    heightFixed = height;

    if (!ensureShadersLoaded())
        return false;
    if (!createPingPong())
        return false;
    if (!createOutputTarget())
        return false;

    return true;
}

GLuint applyBloom(GLuint sceneTex,
                  GLuint brightTex,
                  int iterations,
                  float exposure)
{
    if (sceneTex == 0 || brightTex == 0)
    {
        LOG_ERROR(("applyBloom: invalid inputs (sceneTex=" + std::to_string(sceneTex) + ", brightTex=" + std::to_string(brightTex) + ").").c_str());
        return 0;
    }
    if (!shadersReady || outFbo == 0 || pingFbo[0] == 0 || pingFbo[1] == 0)
    {
        LOG_ERROR("applyBloom: bloom not initialized. Call initBloom(width, height) first.");
        return 0;
    }

    // Use pre-defined 9-tap kernel to generate blur.
    int kernelRadius = 4; // 9-tap
    float sigma = -1.0f;

    GLuint currentSource = brightTex;
    if (iterations < 1)
        iterations = 1;
    for (int i = 0; i < iterations; ++i)
    {
        currentSource = runSeparableGaussianBlur(currentSource, kernelRadius, sigma);
        if (currentSource == 0)
        {
            LOG_ERROR(("applyBloom: blur pass " + std::to_string(i) + " failed.").c_str());
            return 0;
        }
    }

    if (!compositeSceneAndBloom(sceneTex, currentSource, exposure))
    {
        LOG_ERROR("applyBloom: composite failed.");
        return 0;
    }
    return outTex;
}

GLuint applyBloomWithKernel(GLuint sceneTex,
                            GLuint brightTex,
                            int kernelRadius,
                            float sigma,
                            float exposure)
{
    if (sceneTex == 0 || brightTex == 0)
    {
        LOG_ERROR(("applyBloomWithKernel: invalid inputs (sceneTex=" + std::to_string(sceneTex) + ", brightTex=" + std::to_string(brightTex) + ").").c_str());
        return 0;
    }
    if (!shadersReady || outFbo == 0 || pingFbo[0] == 0 || pingFbo[1] == 0)
    {
        LOG_ERROR("applyBloomWithKernel: bloom not initialized. Call initBloom(width, height) first.");
        return 0;
    }

    GLuint blurred = runSeparableGaussianBlur(brightTex, kernelRadius, sigma);
    if (blurred == 0)
    {
        LOG_ERROR("applyBloomWithKernel: blur failed.");
        return 0;
    }
    if (!compositeSceneAndBloom(sceneTex, blurred, exposure))
    {
        LOG_ERROR("applyBloomWithKernel: composite failed.");
        return 0;
    }
    return outTex;
}

void destroyBloomResources()
{
    for (int i = 0; i < 2; ++i)
    {
        if (pingTex[i])
        {
            glDeleteTextures(1, &pingTex[i]);
            pingTex[i] = 0;
        }
        if (pingFbo[i])
        {
            glDeleteFramebuffers(1, &pingFbo[i]);
            pingFbo[i] = 0;
        }
    }
    if (outTex)
    {
        glDeleteTextures(1, &outTex);
        outTex = 0;
    }
    if (outFbo)
    {
        glDeleteFramebuffers(1, &outFbo);
        outFbo = 0;
    }

    widthFixed = heightFixed = 0;
    shadersReady = false;
}
