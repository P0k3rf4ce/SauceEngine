#ifndef RENDER_PROPERTIES_HPP
#define RENDER_PROPERTIES_HPP

#include "modeling/ModelProperties.hpp"
#include <glm/glm.hpp>

namespace rendering {

/**
 * Stores all render related properties of an object
*/
class RenderProperties {
private:
    glm::mat4 modelMatrix; // Cached transformation matrix from GLTF
    
    /**
     * Extract and build the model transformation from GLTF metadata
     */
    void buildModelMatrix(const modeling::ModelProperties &modelProps);

public:
    RenderProperties(const modeling::ModelProperties &modelProps);
    ~RenderProperties();

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
    void update(const modeling::ModelProperties &modelProps, const animation::AnimationProperties &animProps, bool shadow);
};

}

#endif
