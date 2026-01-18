#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <app/Component.hpp>
#include <app/Mesh.hpp>
#include <app/Material.hpp>

#include <vector>
#include <string>
#include <memory>

namespace sauce {

struct MeshEntry {
  Mesh mesh;
  MaterialPtr material;

  MeshEntry() = default;

  MeshEntry(Mesh m, MaterialPtr mat = nullptr)
      : mesh(std::move(m)), material(std::move(mat)) {}
};

class Model : public Component {
public:
  std::vector<MeshEntry> meshes;

  Model() : Component("Model") {}

  explicit Model(std::string modelName)
      : Component(std::move(modelName)) {}

  void addMesh(Mesh mesh, MaterialPtr material = nullptr) {
    meshes.emplace_back(std::move(mesh), std::move(material));
  }

  void uploadToGPU(
      const sauce::PhysicalDevice& physicalDevice,
      const sauce::LogicalDevice& logicalDevice,
      const vk::raii::CommandPool& commandPool,
      const vk::raii::Queue queue
  ) {
    for (auto& entry : meshes) {
      entry.mesh.uploadToGPU(physicalDevice, logicalDevice, commandPool, queue);
    }
  }

  size_t getMeshCount() const { return meshes.size(); }

  MeshEntry& getMesh(size_t index) { return meshes[index]; }
  const MeshEntry& getMesh(size_t index) const { return meshes[index]; }

  void setMaterial(size_t meshIndex, MaterialPtr material) {
    if (meshIndex < meshes.size()) {
      meshes[meshIndex].material = std::move(material);
    }
  }

  void update(float deltaT) override {}
  void render() override {}
};

using ModelPtr = std::shared_ptr<Model>;

}
