#include "rendering/RenderProperties.hpp"
#include <glad/glad.h>
#include <Eigen/Dense>

using namespace rendering;

RenderProperties::RenderProperties(const modeling::ModelProperties &modelProps) {

}

RenderProperties::~RenderProperties() {

}

/**
 * This function is meant to load these 
 * Render properties back into use
*/
void RenderProperties::load() {
    
}

/**
 * This function is meant to remove these 
 * Render properties from use with the
 * intention that they will be used in the future.
*/
void RenderProperties::unload() {

}

/**
 * Run shaders for this object
*/
void RenderProperties::update(const modeling::ModelProperties &modelProps, const animation::AnimationProperties &animProps) {
    
}

/**
 * Generate an irradiance map from a given environment cubemap
*/
unsigned int RenderProperties::genIrradianceMap(unsigned int envCubemap, unsigned int captureFBO, unsigned int captureRBO, Eigen::Matrix4i captureViews[], Eigen::Matrix4i captureProj) {
    // using a small 32x32 cubemap as irradiance map
    const unsigned int IRRADIANCE_SIZE = 32;

    // our irradiance map, which will be the convolution of the environment map
    unsigned int irradianceMap;

    // this glGenTextures call will generate a texture ID and store it in the variable irradianceMap
    glGenTextures(1, &irradianceMap);

    // makes irradianceMap the active cubemap texture so we can configure its faces and parameters
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);

    // allocate each cubemap face with 16 bit floating point RGB format for HDR irradiance sampling
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                     IRRADIANCE_SIZE, IRRADIANCE_SIZE, 0, GL_RGB, GL_FLOAT, nullptr);
    }

    // sets cubemap wrapping to clamp edges and prevent sampling artifacts at face boundaries
    // uses linear filtering to smooth texture sampling when scaling
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // prepare framebuffer for rendering each cubemap face during irradiance convolution
    // ----------------------------------------------------------------------------
    // tells openGL to render to the framebuffer object instead of the default framebuffer (the screen)
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

    // configure the renderbuffer object for depth testing during cubemap face rendering
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);

    // allocate storage for the renderbuffer object
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, IRRADIANCE_SIZE, IRRADIANCE_SIZE);
    // ----------------------------------------------------------------------------

    // TODO: use irradiance shader to convolve envCubemap into irradianceMap
    // ...where is the irradiance shader?
    // - bind irradianceShader
    // - set uniforms: environmentMap, projection, view

    // this will bind the environment map to texture unit 0 so that the shader can sample it
    // may need to change GL_TEXTURE0 to another texture unit if other textures are being used
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    for (unsigned int i = 0; i < 6; ++i) {
        // TODO: set the view matrix uniform in the shader

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // TODO: render a unit cube with inward-facing normals to sample the environment map
        // ...how?
    }

    // this unbinds the framebuffer so we can render to the screen again (?)
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return irradianceMap;
}