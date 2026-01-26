#pragma once

#include "app/modeling/Texture.hpp"
#include "app/modeling/PropertyValue.hpp"
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <memory>

namespace sauce {
namespace modeling {

struct MaterialProperties {
    // PBR Metallic-Roughness properties
    glm::vec4 baseColorFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;

    // Additional properties
    glm::vec3 emissiveFactor = glm::vec3(0.0f, 0.0f, 0.0f);
    float normalScale = 1.0f;
    float occlusionStrength = 1.0f;

    // Alpha mode
    enum class AlphaMode {
        Opaque,
        Mask,
        Blend
    };
    AlphaMode alphaMode = AlphaMode::Opaque;
    float alphaCutoff = 0.5f;

    // Double-sided
    bool doubleSided = false;
};

class Material {
public:
    Material(const std::string& name = "");

    // Getters
    const std::string& getName() const { return name; }
    const MaterialProperties& getProperties() const { return properties; }
    MaterialProperties& getProperties() { return properties; }

    // Texture access
    std::shared_ptr<Texture> getTexture(TextureType type) const;
    void setTexture(TextureType type, std::shared_ptr<Texture> texture);
    bool hasTexture(TextureType type) const;
    const std::unordered_map<TextureType, std::shared_ptr<Texture>>& getTextures() const { return textures; }

    // Metadata access (for GLTF extensions)
    const std::unordered_map<std::string, PropertyValue>& getMetadata() const { return metadata; }
    void setMetadata(const std::string& key, const PropertyValue& value);
    bool hasMetadata(const std::string& key) const;

private:
    std::string name;
    MaterialProperties properties;
    std::unordered_map<TextureType, std::shared_ptr<Texture>> textures;
    std::unordered_map<std::string, PropertyValue> metadata;
};

} // namespace modeling
} // namespace sauce
