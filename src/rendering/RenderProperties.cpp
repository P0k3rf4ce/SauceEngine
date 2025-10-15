#include "rendering/RenderProperties.hpp"
#include "utils/Logger.hpp"

using namespace rendering;

RenderProperties::RenderProperties(const modeling::ModelProperties &modelProps) {
    initShadowResourcesIfEmitter(modelProps);
}

RenderProperties::~RenderProperties() {
    if (depthMapTex != 0) {
        glDeleteTextures(1, &depthMapTex);
        depthMapTex = 0;
    }
    if (depthMapFBO != 0) {
        glDeleteFramebuffers(1, &depthMapFBO);
        depthMapFBO = 0;
    }
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
    if (depthMapTex != 0) {
        glDeleteTextures(1, &depthMapTex);
        depthMapTex = 0;
    }
    if (depthMapFBO != 0) {
        glDeleteFramebuffers(1, &depthMapFBO);
        depthMapFBO = 0;
    }
}

/**
 * Run shaders for this object
*/
void RenderProperties::update(const modeling::ModelProperties &modelProps, const animation::AnimationProperties &animProps) {
    initShadowResourcesIfEmitter(modelProps);
}

void RenderProperties::initShadowResourcesIfEmitter(const modeling::ModelProperties &modelProps) {
    // TODO: Early return if modelProps is not emitter -- property does not exist yet

    if (depthMapFBO != 0 || depthMapTex != 0)
        return;

    glGenFramebuffers(1, &depthMapFBO);
    glGenTextures(1, &depthMapTex);

    glBindTexture(GL_TEXTURE_2D, depthMapTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowWidth, shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // attach depth texture as FBO's depth buffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTex, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    LOG_INFO("Shadow map initialized.");
}
