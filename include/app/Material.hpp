#pragma once

#include <app/Texture.hpp>

#include <string>
#include <memory>

namespace sauce {

struct Material {
  std::string name;

  TexturePtr baseColor;
  TexturePtr normal;
  TexturePtr albedo;
  TexturePtr metallic;
  TexturePtr roughness;
  TexturePtr ambientOcclusion;

  Material() = default;

  explicit Material(std::string materialName)
      : name(std::move(materialName)) {}

  bool hasBaseColor() const { return baseColor != nullptr && baseColor->isValid(); }
  bool hasNormal() const { return normal != nullptr && normal->isValid(); }
  bool hasAlbedo() const { return albedo != nullptr && albedo->isValid(); }
  bool hasMetallic() const { return metallic != nullptr && metallic->isValid(); }
  bool hasRoughness() const { return roughness != nullptr && roughness->isValid(); }
  bool hasAmbientOcclusion() const { return ambientOcclusion != nullptr && ambientOcclusion->isValid(); }
};

using MaterialPtr = std::shared_ptr<Material>;

}
