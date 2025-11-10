#include "modeling/Material.hpp"
#include "utils/Logger.hpp"
#include <iostream>
#include <glad/glad.h>
#include <stb_image.h>

// ============================================================================
// TextureCache Implementation
// ============================================================================

std::shared_ptr<Texture> TextureCache::getDefaultTexture() {
    if (!defaultTexture) {
        // Create a 1x1 white texture
        auto data = std::unique_ptr<const uint8_t[]>(new uint8_t[4]{255, 255, 255, 255});

        // Generate OpenGL texture
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.get());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        defaultTexture = std::make_shared<Texture>(std::move(data), 1, 1, 4, textureID);
        LOG_DEBUG("Created default white texture");
    }
    return defaultTexture;
}

std::shared_ptr<Texture> TextureCache::getTexture(const std::string& path) {
    // Check if already cached
    auto it = textureCache.find(path);
    if (it != textureCache.end()) {
        LOG_DEBUG_F("Using cached texture: %s", path.c_str());
        return it->second;
    }

    // Load the texture from file
    int width, height, channels;
    stbi_set_flip_vertically_on_load(false); // GLTF doesn't need flipping
    unsigned char* imageData = stbi_load(path.c_str(), &width, &height, &channels, 0);

    if (!imageData) {
        LOG_ERROR_F("Failed to load texture: %s - %s", path.c_str(), stbi_failure_reason());
        return getDefaultTexture();
    }

    LOG_INFO_F("Loaded texture: %s (%dx%d, %d channels)", path.c_str(), width, height, channels);

    // Determine OpenGL format
    GLenum format = GL_RGB;
    if (channels == 1) format = GL_RED;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;

    // Generate OpenGL texture
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, imageData);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Copy data to our Texture structure
    size_t dataSize = width * height * channels;
    auto texData = std::unique_ptr<const uint8_t[]>(new uint8_t[dataSize]);
    std::memcpy(const_cast<uint8_t*>(texData.get()), imageData, dataSize);

    stbi_image_free(imageData);

    auto texture = std::make_shared<Texture>(std::move(texData), width, height, channels, textureID);
    textureCache[path] = texture;

    return texture;
}

std::shared_ptr<Texture> TextureCache::getEmbeddedTexture(const aiTexture* aiTex) {
    if (!aiTex) {
        return getDefaultTexture();
    }

    // Create a unique key for this embedded texture
    std::string key = std::string("*embedded_") + std::to_string(reinterpret_cast<uintptr_t>(aiTex));

    // Check cache
    auto it = textureCache.find(key);
    if (it != textureCache.end()) {
        return it->second;
    }

    int width, height, channels;
    unsigned char* imageData = nullptr;

    if (aiTex->mHeight == 0) {
        // Compressed texture (PNG, JPG, etc.)
        imageData = stbi_load_from_memory(
            reinterpret_cast<const unsigned char*>(aiTex->pcData),
            aiTex->mWidth,
            &width, &height, &channels, 0
        );
    } else {
        // Uncompressed ARGB8888 texture
        width = aiTex->mWidth;
        height = aiTex->mHeight;
        channels = 4;

        // Convert aiTexel to raw RGBA data
        size_t dataSize = width * height * 4;
        imageData = new unsigned char[dataSize];
        for (int i = 0; i < width * height; i++) {
            imageData[i * 4 + 0] = aiTex->pcData[i].r;
            imageData[i * 4 + 1] = aiTex->pcData[i].g;
            imageData[i * 4 + 2] = aiTex->pcData[i].b;
            imageData[i * 4 + 3] = aiTex->pcData[i].a;
        }
    }

    if (!imageData) {
        LOG_ERROR("Failed to load embedded texture");
        return getDefaultTexture();
    }

    LOG_INFO_F("Loaded embedded texture (%dx%d, %d channels)", width, height, channels);

    // Generate OpenGL texture
    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, imageData);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Copy to our structure
    size_t dataSize = width * height * channels;
    auto texData = std::unique_ptr<const uint8_t[]>(new uint8_t[dataSize]);
    std::memcpy(const_cast<uint8_t*>(texData.get()), imageData, dataSize);

    if (aiTex->mHeight == 0) {
        stbi_image_free(imageData);
    } else {
        delete[] imageData;
    }

    auto texture = std::make_shared<Texture>(std::move(texData), width, height, channels, textureID);
    textureCache[key] = texture;

    return texture;
}

void TextureCache::clear() {
    textureCache.clear();
    defaultTexture.reset();
}

// ============================================================================
// Material Implementation
// ============================================================================

Material Material::from_aiMaterial(aiMaterial *material) {
    throw std::runtime_error("Use ModelLoader to create materials");
}

// ============================================================================
// MaterialManager Implementation
// ============================================================================

MaterialManager::MaterialManager(aiScene *scene) {
    throw std::runtime_error("Use ModelLoader to create materials");
}

MaterialHandle MaterialHandle::new_unchecked(size_t id) {
    return MaterialHandle{ id };
}

const std::shared_ptr<Material> MaterialManager::get(MaterialHandle handle) const noexcept {
    return materials[handle.id];
}

std::shared_ptr<Texture> MaterialManager::get_texture(int id) {
    return materials[id]->base_color; // Simplified for now
}

std::shared_ptr<Material> MaterialManager::find(std::string name) const {
    for (auto it = materials.begin(); it != materials.end(); ++it) {
        if ((*it)->name == name) {
            return *it;
        }
    }
    throw std::out_of_range(name + " is not a registered material");
}

MaterialManager MaterialManager::from_aiScene(aiScene *scene) {
    throw std::runtime_error("Use ModelLoader to create materials");
}