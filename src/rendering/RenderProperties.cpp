#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "rendering/RenderProperties.hpp"

#include "modeling/Model.hpp"
#include "shared/Scene.hpp"
#include "utils/Logger.hpp"

using namespace rendering;

RenderProperties::RenderProperties(const modeling::ModelProperties &modelProps) 
    : modelMatrix(1.0f) // Initialize to identity matrix
{
    buildModelMatrix(modelProps);
}

RenderProperties::~RenderProperties() {

}

void RenderProperties::load() {

}

void RenderProperties::unload() {

}

void RenderProperties::buildModelMatrix(const modeling::ModelProperties &modelProps) {
    std::shared_ptr<modeling::Model> model = modelProps.getModel();
    if (!model) {
        modelMatrix = glm::mat4(1.0f); // Identity matrix if no model
        return;
    }

    // Extract scale from model metadata
    glm::vec3 scale(1.0f, 1.0f, 1.0f);
    if (model->hasMetadata("scale_x")) {
        scale.x = static_cast<float>(model->getMetadataValue<double>("scale_x"));
    }
    if (model->hasMetadata("scale_y")) {
        scale.y = static_cast<float>(model->getMetadataValue<double>("scale_y"));
    }
    if (model->hasMetadata("scale_z")) {
        scale.z = static_cast<float>(model->getMetadataValue<double>("scale_z"));
    }

    // Extract position from model metadata
    glm::vec3 position(0.0f, 0.0f, 0.0f);
    if (model->hasMetadata("pos_x")) {
        position.x = static_cast<float>(model->getMetadataValue<double>("pos_x"));
    }
    if (model->hasMetadata("pos_y")) {
        position.y = static_cast<float>(model->getMetadataValue<double>("pos_y"));
    }
    if (model->hasMetadata("pos_z")) {
        position.z = static_cast<float>(model->getMetadataValue<double>("pos_z"));
    }

    // Extract rotation from model metadata
    glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f); // w, x, y, z
    if (model->hasMetadata("rot_w") && model->hasMetadata("rot_x") && 
        model->hasMetadata("rot_y") && model->hasMetadata("rot_z")) {
        rotation.w = static_cast<float>(model->getMetadataValue<double>("rot_w"));
        rotation.x = static_cast<float>(model->getMetadataValue<double>("rot_x"));
        rotation.y = static_cast<float>(model->getMetadataValue<double>("rot_y"));
        rotation.z = static_cast<float>(model->getMetadataValue<double>("rot_z"));
    }

    // Build model matrix: Translation * Rotation * Scale
    modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);
    modelMatrix = modelMatrix * glm::mat4_cast(rotation);
    modelMatrix = glm::scale(modelMatrix, scale);
}

/**
 * Run shaders for this object
*/
void RenderProperties::update(const modeling::ModelProperties &modelProps, const animation::AnimationProperties &animProps, bool shadow) {

    auto model = modelProps.getModel();
	auto shader = model->getShader();

    // set model matrix (use cached transformation from GLTF)
    shader->setUniform("model", modelMatrix);

	if (shadow) {
		// draw each mesh
		for (const auto& [mesh, material] : model->getMeshMaterialPairs()) {
		    if (mesh) { glDrawElements(GL_TRIANGLES, mesh->indices.size(), GL_UNSIGNED_INT, 0); }
		}
	} else {
		auto skybox = Scene::get_active_scene()->skybox;

		// set projection matrix
		auto projection = Scene::get_active_scene()->get_camera()->getProjectionMatrix();
		shader->setUniform("projection", projection);

		// draw each mesh
		bool pbr = false; // temp thing
		for (const auto& [mesh, material] : model->getMeshMaterialPairs()) {
			if (mesh) {
				if (pbr) {
					// set pbr textures
					shader->setUniform("albedoMap", material.albedo->id);
					shader->setUniform("normalMap", material.normal->id);
					shader->setUniform("metallicMap", material.metallic->id);
					shader->setUniform("roughnessMap", material.roughness->id);
					shader->setUniform("aoMap", material.ambient_occlusion->id);

					// set pbr lighting maps
					shader->setUniform("irradianceMap", skybox.irradiance);
					shader->setUniform("prefilterMap", skybox.prefilter);
					shader->setUniform("brdfLUT", skybox.brdf);

					// TODO - set shadow textures
				}

				// draw call
				glDrawElements(GL_TRIANGLES, mesh->indices.size(), GL_UNSIGNED_INT, 0);
			}
		}
	}


}

