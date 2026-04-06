#include "app/modeling/GLTFLoader.hpp"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

#include <glm/gtc/type_ptr.hpp>
#include <filesystem>

namespace sauce {
namespace modeling {

GLTFLoader::GLTFLoader()
    : currentGltfModel(nullptr) {
}

GLTFLoader::GLTFLoader(const LoadOptions& options)
    : options(options)
    , currentGltfModel(nullptr) {
}

std::vector<std::shared_ptr<Model>> GLTFLoader::loadModels(const std::string& filePath) {
    tinygltf::Model gltfModel;
    if (!loadGltfFile(filePath, gltfModel)) {
        return {};
    }

    std::vector<std::shared_ptr<Model>> models;
    for (size_t i = 0; i < gltfModel.scenes.size(); ++i) {
        auto model = processScene(gltfModel, static_cast<int>(i));
        if (model) {
            models.push_back(model);
        }
    }

    return models;
}

std::shared_ptr<Model> GLTFLoader::loadModel(const std::string& filePath, size_t sceneIndex) {
    tinygltf::Model gltfModel;
    if (!loadGltfFile(filePath, gltfModel)) {
        return nullptr;
    }

    if (sceneIndex >= gltfModel.scenes.size()) {
        return nullptr;
    }

    auto model = processScene(gltfModel, static_cast<int>(sceneIndex));
    return model;
}

bool GLTFLoader::loadGltfFile(const std::string& filePath, tinygltf::Model& gltfModel) {
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    // Extract base directory for relative texture paths
    std::filesystem::path path(filePath);
    baseDirectory = path.parent_path().string();

    bool ret = false;
    if (filePath.ends_with(".gltf")) {
        ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, filePath);
    } else if (filePath.ends_with(".glb")) {
        ret = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, filePath);
    } else {
        return false;
    }

    if (!ret) {
        return false;
    }

    currentGltfModel = &gltfModel;
    return true;
}

std::shared_ptr<Model> GLTFLoader::processScene(const tinygltf::Model& gltfModel, int sceneIndex) {
    if (sceneIndex < 0 || sceneIndex >= static_cast<int>(gltfModel.scenes.size())) {
        return nullptr;
    }

    parseLightsExtension(gltfModel);

    const auto& scene = gltfModel.scenes[sceneIndex];
    auto model = std::make_shared<Model>();

    // Create root node
    auto rootNode = std::make_shared<ModelNode>("__root__");

    // Process all root nodes in the scene
    for (int nodeIndex : scene.nodes) {
        auto childNode = processNode(gltfModel, nodeIndex);
        if (childNode) {
            rootNode->addChild(childNode);
        }
    }

    model->setRootNode(rootNode);

    return model;
}

std::shared_ptr<ModelNode> GLTFLoader::processNode(const tinygltf::Model& gltfModel, int nodeIndex) {
    if (nodeIndex < 0 || nodeIndex >= static_cast<int>(gltfModel.nodes.size())) {
        return nullptr;
    }

    const auto& gltfNode = gltfModel.nodes[nodeIndex];
    auto node = std::make_shared<ModelNode>(gltfNode.name);

    // Extract transform
    node->getTransform() = extractTransform(gltfNode);

    // Process mesh if present
    if (gltfNode.mesh >= 0 && gltfNode.mesh < static_cast<int>(gltfModel.meshes.size())) {
        const auto& gltfMesh = gltfModel.meshes[gltfNode.mesh];

        for (const auto& primitive : gltfMesh.primitives) {
            auto mesh = processPrimitive(gltfModel, primitive);
            if (mesh) {
                // Get material
                std::shared_ptr<Material> material;
                if (primitive.material >= 0) {
                    material = processMaterial(gltfModel, primitive.material);
                } else {
                    // Default material
                    material = std::make_shared<Material>("default");
                }

                node->addMeshMaterialPair(mesh, material);
            }
        }
    }

    applyNodeLight(gltfNode, node);
    applyNodeCloth(gltfNode, node);

    // Process children
    processNodeChildren(gltfModel, gltfNode, node);

    return node;
}

void GLTFLoader::processNodeChildren(const tinygltf::Model& gltfModel,
                                      const tinygltf::Node& gltfNode,
                                      std::shared_ptr<ModelNode> node) {
    for (int childIndex : gltfNode.children) {
        auto childNode = processNode(gltfModel, childIndex);
        if (childNode) {
            node->addChild(childNode);
        }
    }
}

Transform GLTFLoader::extractTransform(const tinygltf::Node& gltfNode) {
    Transform transform;

    if (!gltfNode.matrix.empty() && gltfNode.matrix.size() == 16) {
        // Matrix format
        glm::mat4 matrix;
        for (int i = 0; i < 16; ++i) {
            matrix[i / 4][i % 4] = static_cast<float>(gltfNode.matrix[i]);
        }
        transform = Transform::fromMatrix(matrix);
    } else {
        // TRS format
        if (!gltfNode.translation.empty() && gltfNode.translation.size() == 3) {
            transform.setTranslation(glm::vec3(
                static_cast<float>(gltfNode.translation[0]),
                static_cast<float>(gltfNode.translation[1]),
                static_cast<float>(gltfNode.translation[2])
            ));
        }

        if (!gltfNode.rotation.empty() && gltfNode.rotation.size() == 4) {
            // GLTF quaternion format: [x, y, z, w]
            transform.setRotation(glm::quat(
                static_cast<float>(gltfNode.rotation[3]),  // w
                static_cast<float>(gltfNode.rotation[0]),  // x
                static_cast<float>(gltfNode.rotation[1]),  // y
                static_cast<float>(gltfNode.rotation[2])   // z
            ));
        }

        if (!gltfNode.scale.empty() && gltfNode.scale.size() == 3) {
            transform.setScale(glm::vec3(
                static_cast<float>(gltfNode.scale[0]),
                static_cast<float>(gltfNode.scale[1]),
                static_cast<float>(gltfNode.scale[2])
            ));
        }
    }

    return transform;
}

std::shared_ptr<Mesh> GLTFLoader::processPrimitive(const tinygltf::Model& gltfModel,
                                                     const tinygltf::Primitive& primitive) {
    // Only support triangles for now
    if (primitive.mode != TINYGLTF_MODE_TRIANGLES) {
        return nullptr;
    }

    std::vector<sauce::Vertex> vertices;
    std::vector<uint32_t> indices;

    // Extract indices
    extractIndices(gltfModel, primitive, indices);
    if (indices.empty()) {
        return nullptr;
    }

    // Determine vertex count from POSITION attribute
    auto posIt = primitive.attributes.find("POSITION");
    if (posIt == primitive.attributes.end()) {
        return nullptr;
    }

    const auto& posAccessor = gltfModel.accessors[posIt->second];
    size_t vertexCount = posAccessor.count;
    vertices.resize(vertexCount);

    // Initialize with defaults
    for (auto& vertex : vertices) {
        vertex.position = glm::vec3(0.0f);
        vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        vertex.texCoords = glm::vec2(0.0f);
        vertex.color = glm::vec3(1.0f);
        vertex.tangent = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    }

    // Extract vertex attributes
    extractVertexAttribute(gltfModel, primitive, "POSITION", vertices, 0);
    extractVertexAttribute(gltfModel, primitive, "NORMAL", vertices, 1);
    extractVertexAttribute(gltfModel, primitive, "TEXCOORD_0", vertices, 2);
    extractVertexAttribute(gltfModel, primitive, "COLOR_0", vertices, 3);
    extractVertexAttribute(gltfModel, primitive, "TANGENT", vertices, 4);

    auto mesh = std::make_shared<Mesh>(vertices, indices);

    // Generate missing attributes
    bool hasNormals = primitive.attributes.find("NORMAL") != primitive.attributes.end();
    bool hasTangents = primitive.attributes.find("TANGENT") != primitive.attributes.end();

    if (!hasNormals && options.generateNormals) {
        mesh->generateNormals();
    }

    if (!hasTangents && options.generateTangents) {
        mesh->generateTangents();
    }

    // Validate mesh
    if (options.validateMeshes && !mesh->isValid()) {
        return nullptr;
    }

    return mesh;
}

void GLTFLoader::extractVertexAttribute(const tinygltf::Model& gltfModel,
                                         const tinygltf::Primitive& primitive,
                                         const std::string& attributeName,
                                         std::vector<sauce::Vertex>& vertices,
                                         int componentIndex) {
    auto it = primitive.attributes.find(attributeName);
    if (it == primitive.attributes.end()) {
        return;
    }

    int accessorIndex = it->second;
    const auto& accessor = gltfModel.accessors[accessorIndex];
    const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
    const auto& buffer = gltfModel.buffers[bufferView.buffer];

    const unsigned char* data = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
    size_t stride = accessor.ByteStride(bufferView);

    for (size_t i = 0; i < accessor.count && i < vertices.size(); ++i) {
        const unsigned char* ptr = data + i * stride;

        if (componentIndex == 0) {  // POSITION
            if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT && accessor.type == TINYGLTF_TYPE_VEC3) {
                const float* fptr = reinterpret_cast<const float*>(ptr);
                vertices[i].position = glm::vec3(fptr[0], fptr[1], fptr[2]);
            }
        } else if (componentIndex == 1) {  // NORMAL
            if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT && accessor.type == TINYGLTF_TYPE_VEC3) {
                const float* fptr = reinterpret_cast<const float*>(ptr);
                vertices[i].normal = glm::vec3(fptr[0], fptr[1], fptr[2]);
            }
        } else if (componentIndex == 2) {  // TEXCOORD_0
            if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT && accessor.type == TINYGLTF_TYPE_VEC2) {
                const float* fptr = reinterpret_cast<const float*>(ptr);
                vertices[i].texCoords = glm::vec2(fptr[0], fptr[1]);
            }
        } else if (componentIndex == 3) {  // COLOR_0
            if (accessor.type == TINYGLTF_TYPE_VEC3) {
                if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                    const float* fptr = reinterpret_cast<const float*>(ptr);
                    vertices[i].color = glm::vec3(fptr[0], fptr[1], fptr[2]);
                }
            } else if (accessor.type == TINYGLTF_TYPE_VEC4) {
                if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                    const float* fptr = reinterpret_cast<const float*>(ptr);
                    vertices[i].color = glm::vec3(fptr[0], fptr[1], fptr[2]);
                }
            }
        } else if (componentIndex == 4) {  // TANGENT
            if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT && accessor.type == TINYGLTF_TYPE_VEC4) {
                const float* fptr = reinterpret_cast<const float*>(ptr);
                vertices[i].tangent = glm::vec4(fptr[0], fptr[1], fptr[2], fptr[3]);
            }
        }
    }
}

void GLTFLoader::extractIndices(const tinygltf::Model& gltfModel,
                                 const tinygltf::Primitive& primitive,
                                 std::vector<uint32_t>& indices) {
    if (primitive.indices < 0) {
        return;
    }

    const auto& accessor = gltfModel.accessors[primitive.indices];
    const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
    const auto& buffer = gltfModel.buffers[bufferView.buffer];

    const unsigned char* data = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;

    indices.resize(accessor.count);

    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        const uint16_t* ptr = reinterpret_cast<const uint16_t*>(data);
        for (size_t i = 0; i < accessor.count; ++i) {
            indices[i] = static_cast<uint32_t>(ptr[i]);
        }
    } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
        const uint32_t* ptr = reinterpret_cast<const uint32_t*>(data);
        std::memcpy(indices.data(), ptr, accessor.count * sizeof(uint32_t));
    } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(data);
        for (size_t i = 0; i < accessor.count; ++i) {
            indices[i] = static_cast<uint32_t>(ptr[i]);
        }
    }
}

std::shared_ptr<Material> GLTFLoader::processMaterial(const tinygltf::Model& gltfModel, int materialIndex) {
    if (materialIndex < 0 || materialIndex >= static_cast<int>(gltfModel.materials.size())) {
        return nullptr;
    }

    const auto& gltfMaterial = gltfModel.materials[materialIndex];
    auto material = std::make_shared<Material>(gltfMaterial.name);

    // PBR metallic-roughness
    if (gltfMaterial.pbrMetallicRoughness.baseColorTexture.index >= 0) {
        auto texture = processTexture(gltfModel,
                                      gltfMaterial.pbrMetallicRoughness.baseColorTexture.index,
                                      TextureType::BaseColor, true);
        if (texture) {
            material->setTexture(TextureType::BaseColor, texture);
        }
    }

    if (gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0) {
        auto texture = processTexture(gltfModel,
                                      gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index,
                                      TextureType::MetallicRoughness, false);
        if (texture) {
            material->setTexture(TextureType::MetallicRoughness, texture);
        }
    }

    if (gltfMaterial.normalTexture.index >= 0) {
        auto texture = processTexture(gltfModel, gltfMaterial.normalTexture.index,
                                      TextureType::Normal, false);
        if (texture) {
            material->setTexture(TextureType::Normal, texture);
        }
    }

    if (gltfMaterial.occlusionTexture.index >= 0) {
        auto texture = processTexture(gltfModel, gltfMaterial.occlusionTexture.index,
                                      TextureType::Occlusion, false);
        if (texture) {
            material->setTexture(TextureType::Occlusion, texture);
        }
    }

    if (gltfMaterial.emissiveTexture.index >= 0) {
        auto texture = processTexture(gltfModel, gltfMaterial.emissiveTexture.index,
                                      TextureType::Emissive, true);
        if (texture) {
            material->setTexture(TextureType::Emissive, texture);
        }
    }

    // Material properties
    auto& props = material->getProperties();

    const auto& bc = gltfMaterial.pbrMetallicRoughness.baseColorFactor;
    props.baseColorFactor = glm::vec4(
        static_cast<float>(bc[0]),
        static_cast<float>(bc[1]),
        static_cast<float>(bc[2]),
        static_cast<float>(bc[3])
    );

    props.metallicFactor = static_cast<float>(gltfMaterial.pbrMetallicRoughness.metallicFactor);
    props.roughnessFactor = static_cast<float>(gltfMaterial.pbrMetallicRoughness.roughnessFactor);

    const auto& ef = gltfMaterial.emissiveFactor;
    props.emissiveFactor = glm::vec3(
        static_cast<float>(ef[0]),
        static_cast<float>(ef[1]),
        static_cast<float>(ef[2])
    );

    props.normalScale = static_cast<float>(gltfMaterial.normalTexture.scale);
    props.occlusionStrength = static_cast<float>(gltfMaterial.occlusionTexture.strength);

    // Alpha mode
    if (gltfMaterial.alphaMode == "OPAQUE") {
        props.alphaMode = MaterialProperties::AlphaMode::Opaque;
    } else if (gltfMaterial.alphaMode == "MASK") {
        props.alphaMode = MaterialProperties::AlphaMode::Mask;
    } else if (gltfMaterial.alphaMode == "BLEND") {
        props.alphaMode = MaterialProperties::AlphaMode::Blend;
    }

    props.alphaCutoff = static_cast<float>(gltfMaterial.alphaCutoff);
    props.doubleSided = gltfMaterial.doubleSided;

    return material;
}

std::shared_ptr<Texture> GLTFLoader::processTexture(const tinygltf::Model& gltfModel,
                                                      int textureIndex,
                                                      TextureType type,
                                                      bool sRGB) {
    if (!options.loadTextures) {
        return textureCache.getDefaultTexture(type);
    }

    if (textureIndex < 0 || textureIndex >= static_cast<int>(gltfModel.textures.size())) {
        return textureCache.getDefaultTexture(type);
    }

    const auto& gltfTexture = gltfModel.textures[textureIndex];

    if (gltfTexture.source < 0) {
        return textureCache.getDefaultTexture(type);
    }

    return processImage(gltfModel, gltfTexture.source, type, sRGB);
}

std::shared_ptr<Texture> GLTFLoader::processImage(const tinygltf::Model& gltfModel,
                                                    int imageIndex,
                                                    TextureType type,
                                                    bool sRGB) {
    if (imageIndex < 0 || imageIndex >= static_cast<int>(gltfModel.images.size())) {
        return textureCache.getDefaultTexture(type);
    }

    const auto& gltfImage = gltfModel.images[imageIndex];

    if (!gltfImage.uri.empty()) {
        // External file
        std::filesystem::path imagePath = std::filesystem::path(baseDirectory) / gltfImage.uri;
        return textureCache.getTexture(imagePath.string(), type, sRGB);
    } else if (!gltfImage.image.empty()) {
        // Embedded image
        return textureCache.getEmbeddedTexture(
            gltfImage.image,
            gltfImage.width,
            gltfImage.height,
            gltfImage.component,
            type,
            sRGB
        );
    }

    return textureCache.getDefaultTexture(type);
}

void GLTFLoader::parseLightsExtension(const tinygltf::Model& gltfModel) {
    parsedLights.clear();

    auto extIt = gltfModel.extensions.find("KHR_lights_punctual");
    if (extIt == gltfModel.extensions.end()) return;

    const auto& extValue = extIt->second;
    if (!extValue.Has("lights") || !extValue.Get("lights").IsArray()) return;

    const auto& lightsArray = extValue.Get("lights");
    for (size_t i = 0; i < lightsArray.ArrayLen(); ++i) {
        const auto& lightVal = lightsArray.Get(static_cast<int>(i));
        LightInfo info{};

        if (lightVal.Has("name") && lightVal.Get("name").IsString()) {
            info.name = lightVal.Get("name").Get<std::string>();
        }

        if (lightVal.Has("type") && lightVal.Get("type").IsString()) {
            const auto& typeStr = lightVal.Get("type").Get<std::string>();
            if (typeStr == "directional") info.type = LightInfo::Type::Directional;
            else if (typeStr == "point")  info.type = LightInfo::Type::Point;
            else if (typeStr == "spot")   info.type = LightInfo::Type::Spot;
        }

        if (lightVal.Has("color") && lightVal.Get("color").IsArray()) {
            const auto& c = lightVal.Get("color");
            if (c.ArrayLen() >= 3) {
                info.color = glm::vec3(
                    static_cast<float>(c.Get(0).IsNumber() ? c.Get(0).GetNumberAsDouble() : 1.0),
                    static_cast<float>(c.Get(1).IsNumber() ? c.Get(1).GetNumberAsDouble() : 1.0),
                    static_cast<float>(c.Get(2).IsNumber() ? c.Get(2).GetNumberAsDouble() : 1.0)
                );
            }
        }

        if (lightVal.Has("intensity") && lightVal.Get("intensity").IsNumber()) {
            info.intensity = static_cast<float>(lightVal.Get("intensity").GetNumberAsDouble());
        }

        if (lightVal.Has("range") && lightVal.Get("range").IsNumber()) {
            info.range = static_cast<float>(lightVal.Get("range").GetNumberAsDouble());
        }

        if (lightVal.Has("spot") && lightVal.Get("spot").IsObject()) {
            const auto& spot = lightVal.Get("spot");
            if (spot.Has("innerConeAngle") && spot.Get("innerConeAngle").IsNumber()) {
                info.innerConeAngle = static_cast<float>(spot.Get("innerConeAngle").GetNumberAsDouble());
            }
            if (spot.Has("outerConeAngle") && spot.Get("outerConeAngle").IsNumber()) {
                info.outerConeAngle = static_cast<float>(spot.Get("outerConeAngle").GetNumberAsDouble());
            }
        }

        parsedLights.push_back(info);
    }
}

void GLTFLoader::applyNodeLight(const tinygltf::Node& gltfNode, std::shared_ptr<ModelNode> node) {
    auto extIt = gltfNode.extensions.find("KHR_lights_punctual");
    if (extIt == gltfNode.extensions.end()) return;

    const auto& extValue = extIt->second;
    if (!extValue.Has("light") || !extValue.Get("light").IsInt()) return;

    int lightIndex = extValue.Get("light").GetNumberAsInt();
    if (lightIndex < 0 || lightIndex >= static_cast<int>(parsedLights.size())) return;

    node->setLightInfo(parsedLights[lightIndex]);
}

void GLTFLoader::applyNodeCloth(const tinygltf::Node& gltfNode, std::shared_ptr<ModelNode> node) {
    auto extIt = gltfNode.extensions.find("SAUCE_cloth");
    if (extIt == gltfNode.extensions.end()) {
        return;
    }

    const auto& extValue = extIt->second;
    ClothInfo clothInfo;

    if (extValue.Has("defaultInvMass") && extValue.Get("defaultInvMass").IsNumber()) {
        clothInfo.settings.defaultInvMass =
            static_cast<float>(extValue.Get("defaultInvMass").GetNumberAsDouble());
    }

    if (extValue.Has("stretchCompliance") && extValue.Get("stretchCompliance").IsNumber()) {
        clothInfo.settings.stretchCompliance =
            static_cast<float>(extValue.Get("stretchCompliance").GetNumberAsDouble());
    }

    if (extValue.Has("bendCompliance") && extValue.Get("bendCompliance").IsNumber()) {
        clothInfo.settings.bendCompliance =
            static_cast<float>(extValue.Get("bendCompliance").GetNumberAsDouble());
    }

    if (extValue.Has("damping") && extValue.Get("damping").IsNumber()) {
        clothInfo.settings.damping =
            static_cast<float>(extValue.Get("damping").GetNumberAsDouble());
    }

    if (extValue.Has("gravityScale") && extValue.Get("gravityScale").IsNumber()) {
        clothInfo.settings.gravityScale =
            static_cast<float>(extValue.Get("gravityScale").GetNumberAsDouble());
    }

    if (extValue.Has("solverSubsteps") && extValue.Get("solverSubsteps").IsInt()) {
        clothInfo.settings.solverSubsteps = extValue.Get("solverSubsteps").GetNumberAsInt();
    } else if (extValue.Has("solverSubsteps") && extValue.Get("solverSubsteps").IsNumber()) {
        clothInfo.settings.solverSubsteps =
            static_cast<int>(extValue.Get("solverSubsteps").GetNumberAsDouble());
    }

    if (extValue.Has("pinnedParticleIndices") && extValue.Get("pinnedParticleIndices").IsArray()) {
        const auto& pinnedIndices = extValue.Get("pinnedParticleIndices");
        clothInfo.settings.pinnedParticleIndices.reserve(pinnedIndices.ArrayLen());
        for (size_t i = 0; i < pinnedIndices.ArrayLen(); ++i) {
            const auto& value = pinnedIndices.Get(static_cast<int>(i));
            if (!value.IsInt() && !value.IsNumber()) {
                continue;
            }

            const int particleIndex = value.GetNumberAsInt();
            if (particleIndex >= 0) {
                clothInfo.settings.pinnedParticleIndices.push_back(
                    static_cast<uint32_t>(particleIndex));
            }
        }
    }

    node->setClothInfo(clothInfo);
}

} // namespace modeling
} // namespace sauce
