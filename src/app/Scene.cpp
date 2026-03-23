#include "app/Scene.hpp"
#include "app/modeling/GLTFLoader.hpp"
#include "app/modeling/GLTFExporter.hpp"
#include "app/components/TransformComponent.hpp"
#include "app/components/MeshRendererComponent.hpp"
#include "app/components/PointLightComponent.hpp"
#include "app/components/DirectionalLightComponent.hpp"
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
    loadGLTFNodeHierarchy(model->getRootNode(), nodeToEntityMap, filePath);

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
        loadGLTFNodeHierarchy(model->getRootNode(), nodeToEntityMap, filePath);
    } else {
        // Flatten all meshes into individual entities
        loadGLTFFlattened(model, filePath);
    }
}

void Scene::loadGLTFNodeHierarchy(std::shared_ptr<modeling::ModelNode> node,
                                   std::unordered_map<modeling::ModelNode*, Entity*>& nodeToEntityMap,
                                   const std::string& filePath) {
    if (!node) {
        return;
    }

    // Skip the artificial root node
    if (node->getName() == "__root__") {
        for (const auto& child : node->getChildren()) {
            loadGLTFNodeHierarchy(child, nodeToEntityMap, filePath);
        }
        return;
    }

    // Create entity for this node
    std::string entityName = node->getName().empty() ? "Node" : node->getName();
    Entity entity(entityName);

    // Scene entities are flat, so bake the node hierarchy into a world-space transform on import.
    entity.addComponent<TransformComponent>(modeling::Transform::fromMatrix(node->getWorldMatrix()));

    // Add MeshRendererComponents for each mesh-material pair
    for (const auto& pair : node->getMeshMaterialPairs()) {
        entity.addComponent<MeshRendererComponent>(pair.mesh, pair.material);
        entity.getComponents<MeshRendererComponent>().back()->setModelPath(filePath);
    }

    if (node->hasLight()) {
        const auto& info = node->getLightInfo().value();
        if (info.type == modeling::LightInfo::Type::Point) {
            entity.addComponent<PointLightComponent>(info.color, info.intensity, info.range);
        } else if (info.type == modeling::LightInfo::Type::Directional) {
            entity.addComponent<DirectionalLightComponent>(info.color, info.intensity);
        }
    }

    // Add entity to scene
    entities.push_back(std::move(entity));
    Entity* entityPtr = &entities.back();

    // Track node -> entity mapping
    nodeToEntityMap[node.get()] = entityPtr;

    // Process children
    for (const auto& child : node->getChildren()) {
        loadGLTFNodeHierarchy(child, nodeToEntityMap, filePath);
    }
}

void Scene::loadGLTFFlattened(std::shared_ptr<modeling::Model> model, const std::string& filePath) {
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
        entity.getComponents<MeshRendererComponent>().back()->setModelPath(filePath);

        entities.push_back(std::move(entity));
    }
}

const std::vector<GPULight>& Scene::collectGPULights() {
    gpuLightBuffer.clear();
    for (auto& entity : entities) {
        if (!entity.getActive()) continue;

        auto* lc = entity.getComponent<LightComponent>();
        if (!lc || !lc->getActive()) continue;

        glm::vec3 worldPos{0.0f};
        glm::vec3 direction{0.0f, 0.0f, -1.0f};
        
        auto* tc = entity.getComponent<TransformComponent>();
        if (tc) {
            worldPos = tc->getTranslation();
            direction = tc->getRotation() * direction;
        }

        GPULight gpuLight = lc->toGPULight(worldPos);

        if (lc->getLightType() == LightType::Directional || lc->getLightType() == LightType::Spot) {
            gpuLight.direction = direction;
        }

        gpuLightBuffer.push_back(gpuLight);
    }
    return gpuLightBuffer;
}

} // namespace sauce
