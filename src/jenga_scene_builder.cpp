#include "app/Scene.hpp"
#include "app/Entity.hpp"
#include "app/Vertex.hpp"
#include "app/components/MeshRendererComponent.hpp"
#include "app/components/TransformComponent.hpp"
#include "app/modeling/GLTFLoader.hpp"
#include "app/modeling/Material.hpp"
#include "app/modeling/Mesh.hpp"
#include "app/modeling/Texture.hpp"
#include "app/modeling/Transform.hpp"

#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/gtc/constants.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace sauce {
namespace {

struct PrimitiveAsset {
  std::shared_ptr<modeling::Mesh> mesh;
  std::shared_ptr<modeling::Material> sourceMaterial;
};

PrimitiveAsset loadPrimitiveAsset(const std::filesystem::path& path) {
  modeling::GLTFLoader loader;
  const auto model = loader.loadModel(path.string());
  if (!model) {
    throw std::runtime_error("Failed to load asset: " + path.string());
  }

  const auto& meshes = model->getAllMeshes();
  const auto& materials = model->getAllMaterials();
  if (meshes.empty() || !meshes.front()) {
    throw std::runtime_error("Asset has no mesh: " + path.string());
  }

  PrimitiveAsset asset;
  asset.mesh = meshes.front();
  if (!materials.empty()) {
    asset.sourceMaterial = materials.front();
  }
  return asset;
}

std::shared_ptr<modeling::Material> makeWoodMaterial(const std::string& name,
                                                     const glm::vec4& tint,
                                                     float roughness,
                                                     float metallic,
                                                     const std::filesystem::path& textureDirectory) {
  auto material = std::make_shared<modeling::Material>(name);
  auto& props = material->getProperties();
  props.baseColorFactor = tint;
  props.metallicFactor = metallic;
  props.roughnessFactor = roughness;
  props.normalScale = 0.8f;
  props.occlusionStrength = 1.0f;

  material->setTexture(
      modeling::TextureType::BaseColor,
      std::make_shared<modeling::Texture>(
          (textureDirectory / "wood_table_001_diff_2k.png").string(),
          modeling::TextureType::BaseColor,
          true));
  material->setTexture(
      modeling::TextureType::Normal,
      std::make_shared<modeling::Texture>(
          (textureDirectory / "wood_table_001_nor_gl_2k.png").string(),
          modeling::TextureType::Normal,
          false));
  material->setTexture(
      modeling::TextureType::MetallicRoughness,
      std::make_shared<modeling::Texture>(
          (textureDirectory / "wood_table_001_rough_2k.png").string(),
          modeling::TextureType::MetallicRoughness,
          false));

  return material;
}

std::shared_ptr<modeling::Mesh> makeTableMesh(float halfX, float halfY, float uvTiling) {
  std::vector<Vertex> vertices = {
      {{-halfX, -halfY, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
      {{ halfX, -halfY, 0.0f}, {0.0f, 0.0f, 1.0f}, {uvTiling, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
      {{ halfX,  halfY, 0.0f}, {0.0f, 0.0f, 1.0f}, {uvTiling, uvTiling}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
      {{-halfX,  halfY, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, uvTiling}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
  };
  std::vector<uint32_t> indices = {0, 1, 2, 0, 2, 3};
  return std::make_shared<modeling::Mesh>(vertices, indices);
}

void replaceAll(std::string& text, const std::string& from, const std::string& to) {
  size_t searchFrom = 0;
  while ((searchFrom = text.find(from, searchFrom)) != std::string::npos) {
    text.replace(searchFrom, from.size(), to);
    searchFrom += to.size();
  }
}

void rewriteWoodTextureUris(const std::filesystem::path& scenePath) {
  std::ifstream input(scenePath, std::ios::binary);
  if (!input) {
    throw std::runtime_error("Failed to reopen scene for URI patch: " + scenePath.string());
  }

  std::string text{
      std::istreambuf_iterator<char>(input),
      std::istreambuf_iterator<char>()};

  static constexpr const char* kWoodTextures[] = {
      "wood_table_001_diff_2k.png",
      "wood_table_001_nor_gl_2k.png",
      "wood_table_001_rough_2k.png",
  };

  for (const char* textureName : kWoodTextures) {
    replaceAll(text, textureName, std::string("textures2/") + textureName);
  }

  std::ofstream output(scenePath, std::ios::binary | std::ios::trunc);
  if (!output) {
    throw std::runtime_error("Failed to write URI-patched scene: " + scenePath.string());
  }
  output << text;
}

void addMeshEntity(Scene& scene,
                   const std::string& name,
                   const std::shared_ptr<modeling::Mesh>& mesh,
                   const std::shared_ptr<modeling::Material>& material,
                   const glm::vec3& translation,
                   const glm::quat& rotation,
                   const glm::vec3& scale) {
  Entity entity(name);
  entity.addComponent<TransformComponent>(modeling::Transform(translation, rotation, scale));
  entity.addComponent<MeshRendererComponent>(mesh, material);
  scene.addEntity(std::move(entity));
}

} // namespace
} // namespace sauce

int main() {
  using namespace sauce;

  try {
    const auto projectRoot = std::filesystem::current_path();
    const auto textureDirectory = std::filesystem::absolute(projectRoot / "testScene" / "textures2");
    const auto cubeAsset = loadPrimitiveAsset((projectRoot / "assets/models/Cube.gltf").string());
    if (!cubeAsset.mesh) {
      throw std::runtime_error("Required primitive meshes are unavailable.");
    }

    CameraCreateInfo cameraInfo{
        .scrWidth = 1280.0f,
        .scrHeight = 720.0f,
    };
    Scene scene(cameraInfo);

    const auto blockMaterial =
        makeWoodMaterial("JengaBlockWood", glm::vec4(1.08f, 0.88f, 0.66f, 1.0f), 0.82f, 0.03f, textureDirectory);
    const auto tableMaterial =
        makeWoodMaterial("JengaTableWood", glm::vec4(0.74f, 0.58f, 0.43f, 1.0f), 0.76f, 0.02f, textureDirectory);
    const auto tableMesh = makeTableMesh(8.0f, 8.0f, 6.0f);

    addMeshEntity(
        scene,
        "Table",
        tableMesh,
        tableMaterial,
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f));

    constexpr int kLayerCount = 8;
    constexpr float kBlockLength = 3.0f;
    constexpr float kBlockWidth = 1.0f;
    constexpr float kBlockHeight = 0.6f;
    constexpr float kLayerGap = 0.0f;
    constexpr float kBlockGap = 0.03f;
    constexpr float kTowerBaseClearance = 0.0f;

    const glm::vec3 blockScale(
        kBlockLength * 0.5f,
        kBlockWidth * 0.5f,
        kBlockHeight * 0.5f);
    const float blockHalfHeight = blockScale.z;
    const float rowOffset = kBlockWidth + kBlockGap;
    const float layerStride = kBlockHeight + kLayerGap;
    const glm::quat rotateQuarterTurn =
        glm::angleAxis(glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f));

    int blockIndex = 0;
    for (int layer = 0; layer < kLayerCount; ++layer) {
      const bool alongX = (layer % 2) == 0;
      const glm::quat rotation = alongX ? glm::quat(1.0f, 0.0f, 0.0f, 0.0f) : rotateQuarterTurn;
      const float z = blockHalfHeight + kTowerBaseClearance + static_cast<float>(layer) * layerStride;

      for (int slot = -1; slot <= 1; ++slot) {
        glm::vec3 position(0.0f, 0.0f, z);
        if (alongX) {
          position.y = static_cast<float>(slot) * rowOffset;
        } else {
          position.x = static_cast<float>(slot) * rowOffset;
        }

        addMeshEntity(
            scene,
            "JengaBlock_" + std::to_string(blockIndex++),
            cubeAsset.mesh,
            blockMaterial,
            position,
            rotation,
            blockScale);
      }
    }

    const std::filesystem::path outputPath =
        std::filesystem::absolute(projectRoot / "testScene" / "jenga_tower.gltf");
    if (!scene.saveToFile(outputPath.string())) {
      throw std::runtime_error("Failed to export scene to " + outputPath.string());
    }

    rewriteWoodTextureUris(outputPath);

    std::cout << "Generated " << outputPath << std::endl;
    return 0;
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
}
