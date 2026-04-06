#include <app/Scene.hpp>
#include <app/components/ClothComponent.hpp>
#include <app/components/MeshRendererComponent.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

std::filesystem::path writeClothFixture(
    const std::filesystem::path& fixtureStem,
    bool twoPrimitives) {
  const std::filesystem::path binPath = fixtureStem;
  const std::filesystem::path gltfPath =
      fixtureStem.parent_path() /
      (fixtureStem.stem().string() + ".gltf");

  const std::vector<float> positions {
      0.0f, 0.0f, 0.0f,
      1.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
  };
  const std::vector<uint32_t> indices { 0, 1, 2 };

  {
    std::ofstream binOut(binPath, std::ios::binary | std::ios::trunc);
    binOut.write(
        reinterpret_cast<const char*>(positions.data()),
        static_cast<std::streamsize>(positions.size() * sizeof(float)));
    binOut.write(
        reinterpret_cast<const char*>(indices.data()),
        static_cast<std::streamsize>(indices.size() * sizeof(uint32_t)));
  }

  const std::string extraPrimitive = twoPrimitives
      ? R"(,
          {
            "attributes": { "POSITION": 0 },
            "indices": 1,
            "mode": 4
          })"
      : "";

  const std::string nodeName =
      twoPrimitives ? "MultiClothNode" : "SingleClothNode";

  std::ofstream gltfOut(gltfPath, std::ios::trunc);
  gltfOut
      << "{\n"
      << "  \"asset\": { \"version\": \"2.0\" },\n"
      << "  \"scene\": 0,\n"
      << "  \"extensionsUsed\": [\"SAUCE_cloth\"],\n"
      << "  \"scenes\": [ { \"nodes\": [0] } ],\n"
      << "  \"nodes\": [\n"
      << "    {\n"
      << "      \"name\": \"" << nodeName << "\",\n"
      << "      \"mesh\": 0,\n"
      << "      \"extensions\": {\n"
      << "        \"SAUCE_cloth\": {\n"
      << "          \"defaultInvMass\": 1.0,\n"
      << "          \"stretchCompliance\": 0.05,\n"
      << "          \"bendCompliance\": 0.01,\n"
      << "          \"damping\": 0.2,\n"
      << "          \"gravityScale\": 0.5,\n"
      << "          \"solverSubsteps\": 2,\n"
      << "          \"pinnedParticleIndices\": [0]\n"
      << "        }\n"
      << "      }\n"
      << "    }\n"
      << "  ],\n"
      << "  \"meshes\": [\n"
      << "    {\n"
      << "      \"primitives\": [\n"
      << "        {\n"
      << "          \"attributes\": { \"POSITION\": 0 },\n"
      << "          \"indices\": 1,\n"
      << "          \"mode\": 4\n"
      << "        }"
      << extraPrimitive << "\n"
      << "      ]\n"
      << "    }\n"
      << "  ],\n"
      << "  \"buffers\": [\n"
      << "    { \"uri\": \"" << binPath.filename().string()
      << "\", \"byteLength\": 48 }\n"
      << "  ],\n"
      << "  \"bufferViews\": [\n"
      << "    { \"buffer\": 0, \"byteOffset\": 0, \"byteLength\": 36, \"target\": 34962 },\n"
      << "    { \"buffer\": 0, \"byteOffset\": 36, \"byteLength\": 12, \"target\": 34963 }\n"
      << "  ],\n"
      << "  \"accessors\": [\n"
      << "    {\n"
      << "      \"bufferView\": 0,\n"
      << "      \"componentType\": 5126,\n"
      << "      \"count\": 3,\n"
      << "      \"type\": \"VEC3\",\n"
      << "      \"min\": [0.0, 0.0, 0.0],\n"
      << "      \"max\": [1.0, 1.0, 0.0]\n"
      << "    },\n"
      << "    {\n"
      << "      \"bufferView\": 1,\n"
      << "      \"componentType\": 5125,\n"
      << "      \"count\": 3,\n"
      << "      \"type\": \"SCALAR\"\n"
      << "    }\n"
      << "  ]\n"
      << "}\n";

  return gltfPath;
}

bool testSinglePrimitiveClothImport(std::vector<std::string>& errors) {
  const auto fixtureStem =
      std::filesystem::temp_directory_path() /
      "sauce_single_cloth_fixture.bin";
  const std::filesystem::path gltfPath =
      writeClothFixture(fixtureStem, false);

  sauce::Scene scene({ .scrWidth = 640, .scrHeight = 480 });
  if (!scene.loadFromFile(gltfPath.string())) {
    errors.push_back("single-primitive cloth scene failed to load");
    return false;
  }

  auto* entity = scene.getEntity("SingleClothNode");
  if (!entity) {
    errors.push_back("single-primitive cloth scene did not create the expected entity");
    return false;
  }

  auto* clothComponent = entity->getComponent<sauce::ClothComponent>();
  auto* meshRenderer = entity->getComponent<sauce::MeshRendererComponent>();
  if (!clothComponent || !clothComponent->hasClothData() || !meshRenderer) {
    errors.push_back("single-primitive cloth scene did not create cloth + mesh renderer components");
    return false;
  }

  if (!clothComponent->getRuntimeMesh() ||
      meshRenderer->getMesh() != clothComponent->getRuntimeMesh() ||
      clothComponent->getRuntimeMesh() == clothComponent->getSourceMesh()) {
    errors.push_back("single-primitive cloth scene did not swap the renderer to runtimeMesh");
    return false;
  }

  const auto& settings = clothComponent->getSettings();
  if (settings.solverSubsteps != 2 || settings.pinnedParticleIndices.size() != 1 ||
      settings.pinnedParticleIndices[0] != 0) {
    errors.push_back("single-primitive cloth scene did not preserve imported cloth settings");
    return false;
  }

  return true;
}

bool testMultiPrimitiveClothSkipped(std::vector<std::string>& errors) {
  const auto fixtureStem =
      std::filesystem::temp_directory_path() /
      "sauce_multi_cloth_fixture.bin";
  const std::filesystem::path gltfPath =
      writeClothFixture(fixtureStem, true);

  sauce::Scene scene({ .scrWidth = 640, .scrHeight = 480 });
  if (!scene.loadFromFile(gltfPath.string())) {
    errors.push_back("multi-primitive cloth scene failed to load");
    return false;
  }

  auto* entity = scene.getEntity("MultiClothNode");
  if (!entity) {
    errors.push_back("multi-primitive cloth scene did not create the expected entity");
    return false;
  }

  if (entity->getComponent<sauce::ClothComponent>() != nullptr) {
    errors.push_back("multi-primitive cloth scene should skip cloth component creation");
    return false;
  }

  if (entity->getComponents<sauce::MeshRendererComponent>().size() != 2) {
    errors.push_back("multi-primitive cloth scene did not preserve both mesh renderers");
    return false;
  }

  return true;
}

} // namespace

int main() {
  std::vector<std::string> errors;

  const bool singlePrimitiveOk = testSinglePrimitiveClothImport(errors);
  const bool multiPrimitiveOk = testMultiPrimitiveClothSkipped(errors);

  if (!errors.empty()) {
    std::cerr << "Cloth scene smoke failed:\n";
    for (const auto& error : errors) {
      std::cerr << "  - " << error << "\n";
    }
    return 1;
  }

  std::cout << "Cloth scene smoke passed\n";
  std::cout << "  single primitive cloth: "
            << (singlePrimitiveOk ? "ok" : "failed") << "\n";
  std::cout << "  multi primitive skip: "
            << (multiPrimitiveOk ? "ok" : "failed") << "\n";
  return 0;
}
