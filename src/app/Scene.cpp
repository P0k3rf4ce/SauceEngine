#include "app/Scene.hpp"
#include "app/modeling/GLTFLoader.hpp"
#include "app/modeling/GLTFExporter.hpp"
#include "app/components/TransformComponent.hpp"
#include "app/components/MeshRendererComponent.hpp"
#include <unordered_map>
#include <iostream>

namespace sauce {

void Scene::addEntity(sauce::Entity&& entity) {
    entities.push_back(std::move(entity));
}

sauce::Entity* Scene::getEntity(const std::string& name) {
    for (auto& entity : entities) {
        if (entity.get_name() == name) {
            return &entity;
        }
    }
    return nullptr;
}

bool Scene::saveToFile(const std::string& filePath) const {
    using namespace sauce::modeling;

    ExportOptions opts;
    // Determine binary mode from extension
    if (filePath.size() >= 4 && filePath.substr(filePath.size() - 4) == ".glb") {
        opts.writeBinary = true;
        opts.embedImages = true;
        opts.embedBuffers = true;
    }

    GLTFExporter exporter(opts);
    bool success = exporter.exportScene(*this, filePath);
    if (success) {
        const_cast<Scene*>(this)->currentFilePath = filePath;
    }
    return success;
}

bool Scene::loadFromFile(const std::string& filePath) {
    using namespace sauce::modeling;

    GLTFLoader loader;
    auto model = loader.loadModel(filePath);

    if (!model || !model->getRootNode()) {
        std::cerr << "Scene::loadFromFile: Failed to load " << filePath << std::endl;
        return false;
    }

    // Clear existing entities
    entities.clear();

    // Load entities from model
    std::unordered_map<ModelNode*, Entity*> nodeToEntityMap;
    loadGLTFNodeHierarchy(model->getRootNode(), nullptr, nodeToEntityMap);

    currentFilePath = filePath;
    return true;
}

void Scene::loadGLTFModel(const std::string& filePath, bool preserveHierarchy) {
    using namespace sauce::modeling;

    // Create loader and load model
    GLTFLoader loader;
    auto model = loader.loadModel(filePath);

    if (!model) {
        return;
    }

    if (!model->getRootNode()) {
        return;
    }

    if (preserveHierarchy) {
        // Create entity tree preserving hierarchy
        std::unordered_map<ModelNode*, Entity*> nodeToEntityMap;
        loadGLTFNodeHierarchy(model->getRootNode(), nullptr, nodeToEntityMap);
    } else {
        // Flatten all meshes into individual entities
        loadGLTFFlattened(model);
    }
}

void Scene::loadGLTFNodeHierarchy(std::shared_ptr<modeling::ModelNode> node,
                                   Entity* parentEntity,
                                   std::unordered_map<modeling::ModelNode*, Entity*>& nodeToEntityMap) {
    if (!node) {
        return;
    }

    // Skip the artificial root node
    if (node->getName() == "__root__") {
        for (const auto& child : node->getChildren()) {
            loadGLTFNodeHierarchy(child, nullptr, nodeToEntityMap);
        }
        return;
    }

    // Create entity for this node
    std::string entityName = node->getName().empty() ? "Node" : node->getName();
    Entity entity(entityName);

    // Add TransformComponent
    entity.addComponent<TransformComponent>(node->getTransform());

    // Add MeshRendererComponents for each mesh-material pair
    for (const auto& pair : node->getMeshMaterialPairs()) {
        entity.addComponent<MeshRendererComponent>(pair.mesh, pair.material);
    }

    // Add entity to scene
    entities.push_back(std::move(entity));
    Entity* entityPtr = &entities.back();

    // Track node -> entity mapping
    nodeToEntityMap[node.get()] = entityPtr;

    // Process children
    for (const auto& child : node->getChildren()) {
        loadGLTFNodeHierarchy(child, entityPtr, nodeToEntityMap);
    }
}

void Scene::loadGLTFFlattened(std::shared_ptr<modeling::Model> model) {
    const auto& allMeshes = model->getAllMeshes();
    const auto& allMaterials = model->getAllMaterials();

    // Create one entity per mesh
    for (size_t i = 0; i < allMeshes.size(); ++i) {
        std::string entityName = "Mesh_" + std::to_string(i);
        Entity entity(entityName);

        // Add TransformComponent with default transform
        entity.addComponent<TransformComponent>();

        // Add MeshRendererComponent
        // Use corresponding material if available, otherwise use first material or create default
        std::shared_ptr<modeling::Material> material;
        if (i < allMaterials.size()) {
            material = allMaterials[i];
        } else if (!allMaterials.empty()) {
            material = allMaterials[0];
        } else {
            material = std::make_shared<modeling::Material>("default");
        }

        entity.addComponent<MeshRendererComponent>(allMeshes[i], material);

        entities.push_back(std::move(entity));
    }
}

} // namespace sauce
