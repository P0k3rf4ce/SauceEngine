#include "app/modeling/GLTFExporter.hpp"
#include "app/Entity.hpp"
#include "app/Scene.hpp"
#include "app/components/MeshRendererComponent.hpp"
#include "app/components/TransformComponent.hpp"
#include "app/modeling/Material.hpp"
#include "app/modeling/Mesh.hpp"
#include "app/modeling/Texture.hpp"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <glm/gtc/quaternion.hpp>
#include <iostream>
#include <tiny_gltf.h>

namespace sauce {
    namespace modeling {

        GLTFExporter::GLTFExporter() = default;

        GLTFExporter::GLTFExporter(const ExportOptions& options) : options(options) {
        }

        bool GLTFExporter::exportScene(const Scene& scene, const std::string& filePath) {
            // Reset state
            meshMap.clear();
            materialMap.clear();
            textureMap.clear();
            imageMap.clear();
            bufferData.clear();

            std::filesystem::path outPath(filePath);
            outputDirectory = outPath.parent_path().string();

            // Ensure output directory exists
            if (!outputDirectory.empty()) {
                std::filesystem::create_directories(outputDirectory);
            }

            tinygltf::Model model;
            buildModel(model, scene);

            tinygltf::TinyGLTF writer;
            bool success = false;

            if (options.writeBinary) {
                success =
                    writer.WriteGltfSceneToFile(&model, filePath, options.embedImages,
                                                options.embedBuffers, options.prettyPrint, true);
            } else {
                success =
                    writer.WriteGltfSceneToFile(&model, filePath, options.embedImages,
                                                options.embedBuffers, options.prettyPrint, false);
            }

            if (!success) {
                std::cerr << "GLTFExporter: Failed to write file: " << filePath << std::endl;
            }

            return success;
        }

        void GLTFExporter::buildModel(tinygltf::Model& model, const Scene& scene) {
            model.asset.version = "2.0";
            model.asset.generator = "SauceEngine GLTFExporter";

            tinygltf::Scene gltfScene;
            gltfScene.name = "Scene";

            const auto& entities = scene.getEntities();
            for (size_t i = 0; i < entities.size(); ++i) {
                int nodeIdx = processEntity(model, entities[i]);
                if (nodeIdx >= 0) {
                    gltfScene.nodes.push_back(nodeIdx);
                }
            }

            model.scenes.push_back(gltfScene);
            model.defaultScene = 0;

            // Finalize buffer
            if (!bufferData.empty()) {
                tinygltf::Buffer buffer;
                buffer.data = bufferData;
                model.buffers.push_back(buffer);
            }
        }

        int GLTFExporter::processEntity(tinygltf::Model& model, const Entity& entity) {
            tinygltf::Node node;
            node.name = entity.get_name();

            // Export transform
            const auto* tc = entity.getComponent<TransformComponent>();
            if (tc) {
                const auto& transform = tc->getTransform();
                glm::vec3 t = transform.getTranslation();
                glm::quat r = transform.getRotation();
                glm::vec3 s = transform.getScale();

                // Only write non-default values
                if (t.x != 0.0f || t.y != 0.0f || t.z != 0.0f) {
                    node.translation = {static_cast<double>(t.x), static_cast<double>(t.y),
                                        static_cast<double>(t.z)};
                }

                // GLTF quaternion order: [x, y, z, w]
                if (r.x != 0.0f || r.y != 0.0f || r.z != 0.0f || r.w != 1.0f) {
                    node.rotation = {static_cast<double>(r.x), static_cast<double>(r.y),
                                     static_cast<double>(r.z), static_cast<double>(r.w)};
                }

                if (s.x != 1.0f || s.y != 1.0f || s.z != 1.0f) {
                    node.scale = {static_cast<double>(s.x), static_cast<double>(s.y),
                                  static_cast<double>(s.z)};
                }
            }

            // Export mesh if entity has MeshRendererComponents
            auto mrcs = entity.getComponents<MeshRendererComponent>();
            if (!mrcs.empty()) {
                int meshIdx = getOrCreateMesh(model, entity);
                if (meshIdx >= 0) {
                    node.mesh = meshIdx;
                }
            }

            int nodeIdx = static_cast<int>(model.nodes.size());
            model.nodes.push_back(node);
            return nodeIdx;
        }

        int GLTFExporter::getOrCreateMesh(tinygltf::Model& model, const Entity& entity) {
            auto mrcs = entity.getComponents<MeshRendererComponent>();
            if (mrcs.empty())
                return -1;

            // Check if all mesh pointers match a previously exported mesh group
            // For simplicity, we always create a new GLTF mesh per entity
            // (GLTF meshes contain primitives, one per MRC)
            tinygltf::Mesh gltfMesh;
            gltfMesh.name = entity.get_name();

            for (const auto* mrc : mrcs) {
                auto mesh = mrc->getMesh();
                if (!mesh || !mesh->isValid())
                    continue;

                tinygltf::Primitive primitive;
                primitive.mode = TINYGLTF_MODE_TRIANGLES;

                const auto& vertices = mesh->getVertices();
                const auto& indices = mesh->getIndices();

                if (vertices.empty() || indices.empty())
                    continue;

                // Compute POSITION min/max bounds
                glm::vec3 posMin(std::numeric_limits<float>::max());
                glm::vec3 posMax(std::numeric_limits<float>::lowest());
                for (const auto& v : vertices) {
                    posMin = glm::min(posMin, v.position);
                    posMax = glm::max(posMax, v.position);
                }

                // Pack POSITION data
                {
                    std::vector<float> posData(vertices.size() * 3);
                    for (size_t i = 0; i < vertices.size(); ++i) {
                        posData[i * 3 + 0] = vertices[i].position.x;
                        posData[i * 3 + 1] = vertices[i].position.y;
                        posData[i * 3 + 2] = vertices[i].position.z;
                    }
                    std::vector<double> minVals = {static_cast<double>(posMin.x),
                                                   static_cast<double>(posMin.y),
                                                   static_cast<double>(posMin.z)};
                    std::vector<double> maxVals = {static_cast<double>(posMax.x),
                                                   static_cast<double>(posMax.y),
                                                   static_cast<double>(posMax.z)};
                    int accIdx = addAccessor(model, posData.data(), vertices.size(),
                                             TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_VEC3,
                                             sizeof(float) * 3, minVals, maxVals);
                    primitive.attributes["POSITION"] = accIdx;
                }

                // Pack NORMAL data
                {
                    std::vector<float> normalData(vertices.size() * 3);
                    for (size_t i = 0; i < vertices.size(); ++i) {
                        normalData[i * 3 + 0] = vertices[i].normal.x;
                        normalData[i * 3 + 1] = vertices[i].normal.y;
                        normalData[i * 3 + 2] = vertices[i].normal.z;
                    }
                    int accIdx = addAccessor(model, normalData.data(), vertices.size(),
                                             TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_VEC3,
                                             sizeof(float) * 3);
                    primitive.attributes["NORMAL"] = accIdx;
                }

                // Pack TEXCOORD_0 data
                {
                    std::vector<float> texData(vertices.size() * 2);
                    for (size_t i = 0; i < vertices.size(); ++i) {
                        texData[i * 2 + 0] = vertices[i].texCoords.x;
                        texData[i * 2 + 1] = vertices[i].texCoords.y;
                    }
                    int accIdx = addAccessor(model, texData.data(), vertices.size(),
                                             TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_VEC2,
                                             sizeof(float) * 2);
                    primitive.attributes["TEXCOORD_0"] = accIdx;
                }

                // Pack TANGENT data
                {
                    std::vector<float> tangentData(vertices.size() * 4);
                    for (size_t i = 0; i < vertices.size(); ++i) {
                        tangentData[i * 4 + 0] = vertices[i].tangent.x;
                        tangentData[i * 4 + 1] = vertices[i].tangent.y;
                        tangentData[i * 4 + 2] = vertices[i].tangent.z;
                        tangentData[i * 4 + 3] = vertices[i].tangent.w;
                    }
                    int accIdx = addAccessor(model, tangentData.data(), vertices.size(),
                                             TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_VEC4,
                                             sizeof(float) * 4);
                    primitive.attributes["TANGENT"] = accIdx;
                }

                // Pack indices
                {
                    int accIdx = addAccessor(model, indices.data(), indices.size(),
                                             TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT,
                                             TINYGLTF_TYPE_SCALAR, sizeof(uint32_t));
                    primitive.indices = accIdx;
                }

                // Material
                auto material = mrc->getMaterial();
                if (material) {
                    int matIdx = getOrCreateMaterial(model, *material);
                    if (matIdx >= 0) {
                        primitive.material = matIdx;
                    }
                }

                gltfMesh.primitives.push_back(primitive);
            }

            if (gltfMesh.primitives.empty())
                return -1;

            int meshIdx = static_cast<int>(model.meshes.size());
            model.meshes.push_back(gltfMesh);
            return meshIdx;
        }

        int GLTFExporter::getOrCreateMaterial(tinygltf::Model& model, const Material& material) {
            const Material* ptr = &material;
            auto it = materialMap.find(ptr);
            if (it != materialMap.end()) {
                return it->second;
            }

            tinygltf::Material gltfMat;
            gltfMat.name = material.getName();

            const auto& props = material.getProperties();

            // PBR metallic-roughness
            gltfMat.pbrMetallicRoughness.baseColorFactor = {
                static_cast<double>(props.baseColorFactor.r),
                static_cast<double>(props.baseColorFactor.g),
                static_cast<double>(props.baseColorFactor.b),
                static_cast<double>(props.baseColorFactor.a)};
            gltfMat.pbrMetallicRoughness.metallicFactor = static_cast<double>(props.metallicFactor);
            gltfMat.pbrMetallicRoughness.roughnessFactor =
                static_cast<double>(props.roughnessFactor);

            // Emissive
            gltfMat.emissiveFactor = {static_cast<double>(props.emissiveFactor.r),
                                      static_cast<double>(props.emissiveFactor.g),
                                      static_cast<double>(props.emissiveFactor.b)};

            // Normal scale
            gltfMat.normalTexture.scale = static_cast<double>(props.normalScale);

            // Occlusion strength
            gltfMat.occlusionTexture.strength = static_cast<double>(props.occlusionStrength);

            // Alpha mode
            switch (props.alphaMode) {
            case MaterialProperties::AlphaMode::Opaque:
                gltfMat.alphaMode = "OPAQUE";
                break;
            case MaterialProperties::AlphaMode::Mask:
                gltfMat.alphaMode = "MASK";
                break;
            case MaterialProperties::AlphaMode::Blend:
                gltfMat.alphaMode = "BLEND";
                break;
            }
            gltfMat.alphaCutoff = static_cast<double>(props.alphaCutoff);
            gltfMat.doubleSided = props.doubleSided;

            // Textures
            const auto& textures = material.getTextures();

            auto setTextureInfo = [&](TextureType type, auto& textureInfo) {
                auto texIt = textures.find(type);
                if (texIt != textures.end() && texIt->second) {
                    int texIdx = getOrCreateTexture(model, *texIt->second);
                    if (texIdx >= 0) {
                        textureInfo.index = texIdx;
                    }
                }
            };

            setTextureInfo(TextureType::BaseColor, gltfMat.pbrMetallicRoughness.baseColorTexture);
            setTextureInfo(TextureType::MetallicRoughness,
                           gltfMat.pbrMetallicRoughness.metallicRoughnessTexture);
            setTextureInfo(TextureType::Normal, gltfMat.normalTexture);
            setTextureInfo(TextureType::Occlusion, gltfMat.occlusionTexture);
            setTextureInfo(TextureType::Emissive, gltfMat.emissiveTexture);

            int matIdx = static_cast<int>(model.materials.size());
            model.materials.push_back(gltfMat);
            materialMap[ptr] = matIdx;
            return matIdx;
        }

        int GLTFExporter::getOrCreateTexture(tinygltf::Model& model, const Texture& texture) {
            const Texture* ptr = &texture;
            auto it = textureMap.find(ptr);
            if (it != textureMap.end()) {
                return it->second;
            }

            int imageIdx = getOrCreateImage(model, texture);
            if (imageIdx < 0)
                return -1;

            tinygltf::Texture gltfTex;
            gltfTex.source = imageIdx;

            int texIdx = static_cast<int>(model.textures.size());
            model.textures.push_back(gltfTex);
            textureMap[ptr] = texIdx;
            return texIdx;
        }

        int GLTFExporter::getOrCreateImage(tinygltf::Model& model, const Texture& texture) {
            const Texture* ptr = &texture;
            auto it = imageMap.find(ptr);
            if (it != imageMap.end()) {
                return it->second;
            }

            tinygltf::Image gltfImage;

            if (!texture.isEmbedded() && !texture.getPath().empty()) {
                // External file - use relative URI
                std::filesystem::path texPath(texture.getPath());
                if (!outputDirectory.empty()) {
                    std::filesystem::path outDir(outputDirectory);
                    auto relPath = std::filesystem::relative(texPath, outDir);
                    // If the relative path goes too far up, just use the filename
                    if (relPath.string().starts_with("..")) {
                        // Copy texture to output directory
                        std::filesystem::path destPath = outDir / texPath.filename();
                        try {
                            if (texPath != destPath && std::filesystem::exists(texPath)) {
                                std::filesystem::copy_file(
                                    texPath, destPath,
                                    std::filesystem::copy_options::skip_existing);
                            }
                        } catch (const std::filesystem::filesystem_error&) {
                            // If copy fails, just reference original filename
                        }
                        gltfImage.uri = texPath.filename().string();
                    } else {
                        gltfImage.uri = relPath.string();
                    }
                } else {
                    gltfImage.uri = texPath.filename().string();
                }
            } else if (texture.isEmbedded()) {
                // For embedded textures, store raw RGBA data in the image
                // TinyGLTF will handle embedding in the buffer for .glb
                auto& texRef = const_cast<Texture&>(texture);
                const auto& data = texRef.getData();
                if (!data.empty()) {
                    gltfImage.width = texture.getWidth();
                    gltfImage.height = texture.getHeight();
                    gltfImage.component = 4; // RGBA
                    gltfImage.bits = 8;
                    gltfImage.pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
                    gltfImage.image = data;

                    // Ensure we have 4-channel data
                    if (texture.getChannels() < 4 &&
                        gltfImage.image.size() ==
                            static_cast<size_t>(texture.getWidth() * texture.getHeight() *
                                                texture.getChannels())) {
                        // Convert to RGBA
                        std::vector<unsigned char> rgba(texture.getWidth() * texture.getHeight() *
                                                        4);
                        int ch = texture.getChannels();
                        for (int i = 0; i < texture.getWidth() * texture.getHeight(); ++i) {
                            rgba[i * 4 + 0] = (ch > 0) ? data[i * ch + 0] : 0;
                            rgba[i * 4 + 1] = (ch > 1) ? data[i * ch + 1] : 0;
                            rgba[i * 4 + 2] = (ch > 2) ? data[i * ch + 2] : 0;
                            rgba[i * 4 + 3] = (ch > 3) ? data[i * ch + 3] : 255;
                        }
                        gltfImage.image = rgba;
                    }
                }
            }

            int imgIdx = static_cast<int>(model.images.size());
            model.images.push_back(gltfImage);
            imageMap[ptr] = imgIdx;
            return imgIdx;
        }

        int GLTFExporter::addAccessor(tinygltf::Model& model, const void* data, size_t count,
                                      int componentType, int type, size_t byteStride,
                                      const std::vector<double>& minValues,
                                      const std::vector<double>& maxValues) {
            // Pad for alignment
            padBuffer(4);

            size_t byteOffset = bufferData.size();
            size_t totalBytes = count * byteStride;

            // Append data to buffer
            bufferData.resize(bufferData.size() + totalBytes);
            std::memcpy(bufferData.data() + byteOffset, data, totalBytes);

            // Create buffer view
            tinygltf::BufferView bufferView;
            bufferView.buffer = 0;
            bufferView.byteOffset = byteOffset;
            bufferView.byteLength = totalBytes;

            // Set target based on type
            if (componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT ||
                componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ||
                componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                if (type == TINYGLTF_TYPE_SCALAR) {
                    bufferView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
                }
            } else {
                bufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
            }

            int bufferViewIdx = static_cast<int>(model.bufferViews.size());
            model.bufferViews.push_back(bufferView);

            // Create accessor
            tinygltf::Accessor accessor;
            accessor.bufferView = bufferViewIdx;
            accessor.byteOffset = 0;
            accessor.componentType = componentType;
            accessor.count = count;
            accessor.type = type;

            if (!minValues.empty())
                accessor.minValues = minValues;
            if (!maxValues.empty())
                accessor.maxValues = maxValues;

            int accessorIdx = static_cast<int>(model.accessors.size());
            model.accessors.push_back(accessor);
            return accessorIdx;
        }

        void GLTFExporter::padBuffer(size_t alignment) {
            size_t remainder = bufferData.size() % alignment;
            if (remainder != 0) {
                size_t padding = alignment - remainder;
                bufferData.resize(bufferData.size() + padding, 0);
            }
        }

    } // namespace modeling
} // namespace sauce
