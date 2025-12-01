#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "rendering/RenderProperties.hpp"

#include "modeling/Model.hpp"
#include "shared/Scene.hpp"

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

    // retrieve model
    std::shared_ptr<modeling::Model> model = modelProps.getModel();

    // set pbr stuff - currently not a thing
    // set shadow stuff - currently not a thing

    // set model matrix
    model->getShader()->setUniform("model", animProps.getModelMatrix());

    // set projection matrix(?)
    glm::mat4 projection = glm::perspective(
            glm::radians(Scene::get_active_scene()->get_camera()->getFOV()),
            (float)Scene::scr_width / (float)Scene::scr_height,
            0.1f,
            100.0f
    );
    model->getShader()->setUniform("projection", projection);

    // set textures - TODO
    
    // draw each mesh
    for (const auto& [mesh, material] : model->getMeshMaterialPairs()) {
        if (mesh) {
            glDrawArrays(GL_TRIANGLES, 0, mesh->vertices.size());
        }
    }

}
