#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "rendering/RenderProperties.hpp"

#include "modeling/Model.hpp"
#include "shared/Scene.hpp"
#include "utils/Logger.hpp"

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
    Eigen::Vector3d push(0.0, -1.0, 0.0);
    //model->getShader()->setUniform("model", animProps.getModelMatrix().scale(0.5f).translate(push));
	glm::mat4 mat = glm::translate(glm::mat4(0.2f), glm::vec3(-1.0f, 0.0f, -5.0f));
	model->getShader()->setUniform("model", mat);

    LOG_DEBUG("rendering: model uniform set");

    // set projection matrix(?)
    glm::mat4 projection = glm::perspective(
            //glm::radians(Scene::get_active_scene()->get_camera()->getFOV()),
            glm::radians(90.0f),
            (float)Scene::scr_width / (float)Scene::scr_height,
            0.1f,
            100.0f
    );
	LOG_DEBUG("rendering: binding proj matrix");
    model->getShader()->setUniform("projection", projection);

    // LOG_DEBUG("proj uniform set (supposedly)");

    // get textures - TODO
    //std::vector<modeling::MeshMaterialPair> mats = model->getMeshMaterialPairs();
    
    // draw each mesh
    for (const auto& [mesh, material] : model->getMeshMaterialPairs()) {
        if (mesh) {
			LOG_DEBUG_F("attempting to draw %u vertices", mesh->vertices.size());
            glDrawElements(GL_TRIANGLES, mesh->indices.size(), GL_UNSIGNED_INT, 0);
        }
    }

}
