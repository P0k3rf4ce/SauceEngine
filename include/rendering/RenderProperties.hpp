#ifndef RENDER_PROPERTIES_HPP
#define RENDER_PROPERTIES_HPP

#include "modeling/ModelProperties.hpp"
#include <glad/glad.h>

namespace rendering {

/**
 * Stores all render related properties of an object
*/
class RenderProperties {
public:
    RenderProperties(const modeling::ModelProperties &modelProps);
    ~RenderProperties();

    GLuint getShadowDepthTexture() const {return depthMapTex;}
    GLuint getShadowFrameBuffer() const {return depthMapFBO;}

    /**
     * This function is meant to load these 
     * Render properties back into use
    */
    void load();

    /**
     * This function is meant to remove these 
     * Render properties from use with the
     * intention that they will be used in the future.
    */
    void unload();

    /**
     * Run shaders for this object
    */
    void update(const modeling::ModelProperties &modelProps, const animation::AnimationProperties &animProps);

private:
    // Shadow Mapping
    GLuint depthMapFBO = 0;
    GLuint depthMapTex = 0;
    int shadowWidth = 1024;
    int shadowHeight = 1024;

    void initShadowResourcesIfEmitter(const modeling::ModelProperties &modelProps);
};

}

#endif
