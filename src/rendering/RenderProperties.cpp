#include "rendering/RenderProperties.hpp"

using namespace rendering;

RenderProperties::RenderProperties(const modeling::ModelProperties &modelProps) {

}

RenderProperties::~RenderProperties() {

}

void RenderProperties::load() {

}

void RenderProperties::unload() {

}

/**
 * Run shaders for this object
*/
void RenderProperties::update(const modeling::ModelProperties &modelProps, const animation::AnimationProperties &animProps) {

    // set pbr stuff - currently not a thing
    // set shadow stuff - currently not a thing

    // set model matrix
    // set projection matrix(?)
    // set textures
    
    // draw each mesh
    std::vector<std::shared_ptr<Mesh>> meshes = modelProps.getMeshes;
    for (const std::shared_ptr<Mesh>& mesh : meshes) {
        glDrawArrays(GL_TRIANGLES, 0, mesh->vertices.size());
    }

}
