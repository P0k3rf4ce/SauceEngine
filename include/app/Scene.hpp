#pragma once

#include <app/Camera.hpp>
#include <app/Entity.hpp>

namespace sauce {

class SauceEngineApp;

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
   * Returns a const ref to camera
   */
  const sauce::Camera& getCameraRO() const noexcept {
    return *pCamera;
  }

private:
  std::vector<sauce::Entity> entities;

  std::unique_ptr<sauce::Camera> pCamera;

  /**
   * Returns a non-const ref to camera
   */
  sauce::Camera& getCameraRW() noexcept {
    return *pCamera;
  }

  friend class sauce::SauceEngineApp; // This gives the app class full access to entities and camera for user interaction
};

}
