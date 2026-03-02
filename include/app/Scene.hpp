#pragma once

#include <app/Camera.hpp>
#include <app/Entity.hpp>
#include <memory>
#include <string>
#include <unordered_map>

#include <physics/XPBD.hpp>

namespace sauce {

namespace modeling {
  class ModelNode;
  class Model;
}

class SauceEngineApp;

namespace editor {
  class EditorApp;
}

class Scene {
public:
  /**
   * Loads scene from the file if provided. Otherwise, creates an empty scene
   * @param cameraCreateInfo - arguments for camera creation
   * @param filename - file to load from
   */
  Scene(const sauce::CameraCreateInfo& cameraCreateInfo, const std::string& filename = "") {
    pCamera = std::make_unique<sauce::Camera>( cameraCreateInfo );
  }

  const std::vector<sauce::Entity>& getEntities() const {
    return entities;
  }

  /**
   * Adds an entity to the scene
   */
  void addEntity(sauce::Entity&& entity);

  /**
   * Gets an entity by name (returns nullptr if not found)
   */
  sauce::Entity* getEntity(const std::string& name);

  /**
   * Loads a GLTF model and creates entities with components
   * @param filePath - path to the GLTF file
   * @param preserveHierarchy - if true, creates entity tree; if false, flattens to single level
   */
  void loadGLTFModel(const std::string& filePath, bool preserveHierarchy = true);

  /**
   * Saves the entire scene to a GLTF/GLB file
   * @return true on success
   */
  bool saveToFile(const std::string& filePath) const;

  /**
   * Loads a GLTF/GLB file as the entire scene (clears existing entities)
   * @return true on success
   */
  bool loadFromFile(const std::string& filePath);

  const std::string& getCurrentFilePath() const { return currentFilePath; }
  void setCurrentFilePath(const std::string& path) { currentFilePath = path; }
  bool hasFilePath() const { return !currentFilePath.empty(); }

  /**
   * Returns a const ref to camera
   */
  const sauce::Camera& getCameraRO() const noexcept {
    return *pCamera;
  }

  std::vector<sauce::Entity>& getEntitiesMut() {
    return entities;
  }

private:
  std::vector<sauce::Entity> entities;

  std::unique_ptr<sauce::Camera> pCamera;

  std::string currentFilePath;

  // Helper functions for GLTF loading
  void loadGLTFNodeHierarchy(std::shared_ptr<modeling::ModelNode> node,
                             Entity* parentEntity,
                             std::unordered_map<modeling::ModelNode*, Entity*>& nodeToEntityMap,
                             const std::string& filePath);
  void loadGLTFFlattened(std::shared_ptr<modeling::Model> model, const std::string& filePath);

  /**
   * Returns a non-const ref to camera
   */
  sauce::Camera& getCameraRW() noexcept {
    return *pCamera;
  }

  friend class sauce::SauceEngineApp; // This gives the app class full access to entities and camera for user interaction
  friend class sauce::editor::EditorApp;
};

}
