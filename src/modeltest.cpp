#include "app/modeling/GLTFLoader.hpp"
#include "app/modeling/Model.hpp"
#include "app/modeling/ModelNode.hpp"
#include "app/modeling/Mesh.hpp"
#include "app/modeling/Material.hpp"
#include "app/modeling/Texture.hpp"
#include <iostream>
#include <iomanip>
#include <memory>

using namespace sauce::modeling;

std::string textureTypeToString(TextureType type) {
    switch (type) {
        case TextureType::BaseColor: return "BaseColor";
        case TextureType::Normal: return "Normal";
        case TextureType::MetallicRoughness: return "MetallicRoughness";
        case TextureType::Occlusion: return "Occlusion";
        case TextureType::Emissive: return "Emissive";
        case TextureType::Unknown: return "Unknown";
        default: return "Invalid";
    }
}

std::string alphaModeToString(MaterialProperties::AlphaMode mode) {
    switch (mode) {
        case MaterialProperties::AlphaMode::Opaque: return "Opaque";
        case MaterialProperties::AlphaMode::Mask: return "Mask";
        case MaterialProperties::AlphaMode::Blend: return "Blend";
        default: return "Unknown";
    }
}

void printMeshInfo(const std::shared_ptr<Mesh>& mesh, int index) {
    std::cout << "  Mesh #" << index << ":\n";
    std::cout << "    Vertices: " << mesh->getVertexCount() << "\n";
    std::cout << "    Indices: " << mesh->getIndexCount() << "\n";
    std::cout << "    Triangles: " << (mesh->getIndexCount() / 3) << "\n";
    std::cout << "    Valid: " << (mesh->isValid() ? "Yes" : "No") << "\n";
    std::cout << "    Has GPU Data: " << (mesh->hasGPUData() ? "Yes" : "No") << "\n";

    if (mesh->getVertexCount() > 0) {
        const auto& vertices = mesh->getVertices();
        std::cout << "    First vertex position: ("
                  << vertices[0].position.x << ", "
                  << vertices[0].position.y << ", "
                  << vertices[0].position.z << ")\n";
    }
}

void printMaterialInfo(const std::shared_ptr<Material>& material, int index) {
    std::cout << "  Material #" << index << ":\n";
    std::cout << "    Name: " << material->getName() << "\n";

    const auto& props = material->getProperties();
    std::cout << "    Base Color: ("
              << props.baseColorFactor.r << ", "
              << props.baseColorFactor.g << ", "
              << props.baseColorFactor.b << ", "
              << props.baseColorFactor.a << ")\n";
    std::cout << "    Metallic: " << props.metallicFactor << "\n";
    std::cout << "    Roughness: " << props.roughnessFactor << "\n";
    std::cout << "    Emissive: ("
              << props.emissiveFactor.r << ", "
              << props.emissiveFactor.g << ", "
              << props.emissiveFactor.b << ")\n";
    std::cout << "    Alpha Mode: " << alphaModeToString(props.alphaMode) << "\n";
    std::cout << "    Double Sided: " << (props.doubleSided ? "Yes" : "No") << "\n";

    const auto& textures = material->getTextures();
    if (!textures.empty()) {
        std::cout << "    Textures:\n";
        for (const auto& [type, texture] : textures) {
            std::cout << "      " << textureTypeToString(type) << ": ";
            if (texture->isEmbedded()) {
                std::cout << "embedded (" << texture->getWidth() << "x" << texture->getHeight()
                          << ", " << texture->getChannels() << " channels)";
            } else {
                std::cout << texture->getPath();
            }
            std::cout << " [sRGB: " << (texture->isSRGB() ? "Yes" : "No") << "]";
            std::cout << " [GPU: " << (texture->hasGPUData() ? "Yes" : "No") << "]\n";
        }
    }
}

void printNodeHierarchy(const std::shared_ptr<ModelNode>& node, int depth = 0) {
    std::string indent(depth * 2, ' ');

    std::cout << indent << "Node: " << node->getName() << "\n";

    const auto& transform = node->getTransform();
    std::cout << indent << "  Translation: ("
              << transform.getTranslation().x << ", "
              << transform.getTranslation().y << ", "
              << transform.getTranslation().z << ")\n";
    std::cout << indent << "  Scale: ("
              << transform.getScale().x << ", "
              << transform.getScale().y << ", "
              << transform.getScale().z << ")\n";

    const auto& pairs = node->getMeshMaterialPairs();
    if (!pairs.empty()) {
        std::cout << indent << "  Mesh-Material Pairs: " << pairs.size() << "\n";
        for (size_t i = 0; i < pairs.size(); ++i) {
            std::cout << indent << "    Pair #" << i << ": "
                      << "Mesh vertices=" << pairs[i].mesh->getVertexCount()
                      << ", Material=" << pairs[i].material->getName() << "\n";
        }
    }

    const auto& children = node->getChildren();
    if (!children.empty()) {
        std::cout << indent << "  Children: " << children.size() << "\n";
        for (const auto& child : children) {
            printNodeHierarchy(child, depth + 1);
        }
    }
}

int main(int argc, char* argv[]) {
    std::cout << "=== Model Loader Test Program ===\n\n";

    std::string modelPath = "/home/noah/Desktop/projects/SauceEngine/assets/models/monkey.gltf";

    if (argc > 1) {
        modelPath = argv[1];
    }

    std::cout << "Loading model: " << modelPath << "\n\n";

    try {
        LoadOptions options;
        options.generateNormals = true;
        options.generateTangents = true;
        options.validateMeshes = true;
        options.loadTextures = true;

        GLTFLoader loader(options);
        auto model = loader.loadModel(modelPath);

        if (!model) {
            std::cerr << "ERROR: Failed to load model\n";
            return 1;
        }

        std::cout << "SUCCESS: Model loaded successfully!\n\n";

        std::cout << "=== Model Statistics ===\n";
        const auto& meshes = model->getAllMeshes();
        const auto& materials = model->getAllMaterials();

        std::cout << "Total Meshes: " << meshes.size() << "\n";
        std::cout << "Total Materials: " << materials.size() << "\n\n";

        if (!meshes.empty()) {
            std::cout << "=== Mesh Details ===\n";
            for (size_t i = 0; i < meshes.size(); ++i) {
                printMeshInfo(meshes[i], i);
                std::cout << "\n";
            }
        }

        if (!materials.empty()) {
            std::cout << "=== Material Details ===\n";
            for (size_t i = 0; i < materials.size(); ++i) {
                printMaterialInfo(materials[i], i);
                std::cout << "\n";
            }
        }

        std::cout << "=== Node Hierarchy ===\n";
        if (model->getRootNode()) {
            printNodeHierarchy(model->getRootNode());
        } else {
            std::cout << "No root node found\n";
        }

        std::cout << "\n=== Test Complete ===\n";
        std::cout << "Model loaded successfully without Vulkan initialization!\n";

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "ERROR: Exception caught: " << e.what() << "\n";
        return 1;
    }
}
