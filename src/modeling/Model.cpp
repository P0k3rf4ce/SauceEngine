#include "modeling/Model.hpp"

using namespace modeling;

Model::Model() {
    // Initialize empty vectors - they're already default constructed
}

Model::Model(std::vector<std::shared_ptr<Mesh>> meshes, std::vector<std::shared_ptr<Material>> mats, std::shared_ptr<Shader> shader)
    : meshes(meshes), materials(mats), shader(shader) {
    // All members initialized in initializer list
}

Model::~Model() {
    // Vectors will automatically clean up their contents
    // No explicit cleanup needed for Mesh and Material objects
}

std::vector<std::shared_ptr<Mesh>> Model::getMeshes() const {
    return meshes;
}

std::vector<std::shared_ptr<Material>> Model::getMaterials() const {
    return materials;
}

void Model::addMesh(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material) {
    meshes.push_back(mesh);
    materials.push_back(material);
}

std::shared_ptr<Shader> Model::getShader() {
    return shader;
}

void Model::setupForRendering() {
    if (shader) {
        shader->bind();
    }

    // Bind vertex data for all meshes (rendering team will handle drawing)
    for (size_t i = 0; i < meshes.size(); ++i) {
        if (meshes[i]) {
            meshes[i]->bind();
        }
    }
}

void Model::setMetadata(const std::unordered_map<std::string, PropertyValue>& data) {
    metadata = data;
}

const std::unordered_map<std::string, PropertyValue>& Model::getMetadata() const {
    return metadata;
}