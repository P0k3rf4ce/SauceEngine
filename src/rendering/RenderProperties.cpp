#include "rendering/RenderProperties.hpp"


using namespace rendering;

RenderProperties::RenderProperties(const modeling::ModelProperties &modelProps) {
}

RenderProperties::~RenderProperties() {

}
void RenderProperties::load() {}
void RenderProperties::unload() {}

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
void RenderProperties::update(const modeling::ModelProperties &modelProps, const animation::AnimationProperties &animProps, const bool shadow = false) {
    Shader& activeShader = shadow ? this->shadowShader : this->pbrShader;
    activeShader.bind();

    if (shadow) {
        activeShader.setUniform("lightSpaceMatrix", light.getLightSpaceMatrix());
        activeShader.setUniform("model", animProps.getModelMatrix());
    } else {
        activeShader.setUniform("projection", camera.getProjectionMatrix());
        activeShader.setUniform("view", camera.getViewMatrix());
        activeShader.setUniform("model", animProps.getModelMatrix());
        activeShader.setUniform("camPos", camera.getPosition());
        activeShader.setUniform("lightPos", light.getPosition());
        activeShader.setUniform("lightSpaceMatrix", light.getLightSpaceMatrix());

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_CUBE_MAP, this->irradianceMap);
        activeShader.setUniform("irradianceMap", 5);

        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_CUBE_MAP, this->prefilterMap);
        activeShader.setUniform("prefilterMap", 6);

        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, this->brdfLUT);
        activeShader.setUniform("brdfLUT", 7);

        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D, light.getShadowMapTextureID());
        activeShader.setUniform("shadowMap", 8);
    }
    for (const auto& mesh : modelProps.getMeshes()) {
        if (!shadow) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh.albedoMapID);
            activeShader.setUniform("albedoMap", 0);
            
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, mesh.normalMapID);
            activeShader.setUniform("normalMap", 1);
            
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, mesh.metallicRoughnessMapID);
            activeShader.setUniform("metallicRoughnessMap", 2);
        }

        glBindVertexArray(mesh.VAO);
        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    activeShader.unbind();
}
