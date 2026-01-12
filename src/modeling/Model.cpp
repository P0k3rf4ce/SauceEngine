#include "modeling/Model.hpp"
//#include "utils/Logger.hpp"

using namespace modeling;

Model::Model() {
    // Initialize empty vector - it's already default constructed
}

Model::Model(std::vector<MeshMaterialPair> meshMaterials, std::shared_ptr<Shader> shader)
    : meshMaterials(std::move(meshMaterials)), shader(shader) {
    // All members initialized in initializer list
}

Model::~Model() {
    // Vector will automatically clean up its contents
    // No explicit cleanup needed for Mesh and Material objects
}

const std::vector<MeshMaterialPair>& Model::getMeshMaterialPairs() const {
    return meshMaterials;
}

void Model::addMesh(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material) {
    meshMaterials.emplace_back(std::move(mesh), std::move(material));
}

std::shared_ptr<Shader> Model::getShader() {
    return shader;
}

void Model::setupForRendering() {
    if (shader) {
        shader->bind();
    }

    // Bind vertex data for all meshes (rendering team will handle drawing)
    for (const auto& [mesh, material] : meshMaterials) {
        if (mesh) {
            mesh->bind();
        }
    }
}

void Model::setMetadata(const std::unordered_map<std::string, PropertyValue>& data) {
    metadata = data;
}

const std::unordered_map<std::string, PropertyValue>& Model::getMetadata() const {
    return metadata;
}
