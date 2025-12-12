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
	//glm::mat4 mat = glm::translate(glm::mat4(0.2f), glm::vec3(-1.0f, 0.0f, -5.0f));
	glm::mat4 mat = glm::mat4(0.2f);
	model->getShader()->setUniform("model", mat);


    // set projection matrix(?)
    glm::mat4 projection = Scene::get_active_scene()->get_camera()->getProjectionMatrix();
    model->getShader()->setUniform("projection", projection);


    // get textures - TODO
    //std::vector<modeling::MeshMaterialPair> mats = model->getMeshMaterialPairs();
    
    // draw each mesh
    for (const auto& [mesh, material] : model->getMeshMaterialPairs()) {
        if (mesh) {
            for (const auto& vertex : mesh->vertices) {
                LOG_DEBUG_F("vertex: %f, %f, %f", vertex.Position.x, vertex.Position.y, vertex.Position.z);
            }
            for (const auto& index : mesh->indices) {
                LOG_DEBUG_F("index: %u", index);
            }
            glDrawElements(GL_TRIANGLES, mesh->indices.size(), GL_UNSIGNED_INT, 0);
        }
    }

}
