#include "app/modeling/Material.hpp"

namespace sauce {
namespace modeling {

Material::Material(const std::string& name)
    : name(name) {
}

std::shared_ptr<Texture> Material::getTexture(TextureType type) const {
    auto it = textures.find(type);
    if (it != textures.end()) {
        return it->second;
    }
    return nullptr;
}

void Material::setTexture(TextureType type, std::shared_ptr<Texture> texture) {
    textures[type] = texture;
}

bool Material::hasTexture(TextureType type) const {
    return textures.find(type) != textures.end();
}

void Material::setMetadata(const std::string& key, const PropertyValue& value) {
    metadata[key] = value;
}

bool Material::hasMetadata(const std::string& key) const {
    return metadata.find(key) != metadata.end();
}

} // namespace modeling
} // namespace sauce
