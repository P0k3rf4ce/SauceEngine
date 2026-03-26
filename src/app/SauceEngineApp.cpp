#include <app/SauceEngineApp.hpp>
#include <app/ui/components/HelloWorldWindow.hpp>
#include <app/ui/components/DebugStatsWindow.hpp>
#include <app/components/TransformComponent.hpp>
#include <app/components/RigidBodyComponent.hpp>
#include <app/components/MeshRendererComponent.hpp>
#include <app/components/ClothComponent.hpp>
#include <app/components/LightComponent.hpp>
#include <functional>
#include <cstring>
#include <limits>

#include <physics/XPBD.hpp>
#include <physics/constraints/Constraint.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

namespace sauce {

namespace {

struct SceneBounds {
  glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
  glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());
  bool valid = false;
};

bool isStaticTableMesh(const MeshRendererComponent* meshRenderer) {
  return meshRenderer && meshRenderer->getMesh() && meshRenderer->getMesh()->getVertexCount() == 4;
}

glm::quat integrateAngularVelocity(const glm::quat& orientation,
                                   const glm::vec3& angularVelocity,
                                   float deltaTime) {
  const glm::quat spin(0.0f, angularVelocity.x, angularVelocity.y, angularVelocity.z);
  return glm::normalize(orientation + 0.5f * spin * orientation * deltaTime);
}

float computeScaledMeshRadius(const modeling::Mesh& mesh,
                              const glm::vec3& centerOfMass,
                              const glm::vec3& scale) {
  float maxRadiusSq = 0.0f;
  for (const auto& vertex : mesh.getVertices()) {
    const glm::vec3 localOffset = (vertex.position - centerOfMass) * scale;
    maxRadiusSq = std::max(maxRadiusSq, glm::length2(localOffset));
  }

  return std::sqrt(maxRadiusSq);
}

void expandSceneBounds(SceneBounds& bounds,
                       const modeling::Mesh& mesh,
                       const glm::mat4& modelMatrix) {
  for (const auto& vertex : mesh.getVertices()) {
    const glm::vec3 worldPosition = glm::vec3(modelMatrix * glm::vec4(vertex.position, 1.0f));
    bounds.min = glm::min(bounds.min, worldPosition);
    bounds.max = glm::max(bounds.max, worldPosition);
    bounds.valid = true;
  }
}

} // namespace

SauceEngineApp::SauceEngineApp() {
  pImGuiComponentManager = std::make_unique<sauce::ui::ImGuiComponentManager>();
}

SauceEngineApp::~SauceEngineApp() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void SauceEngineApp::run(const uint32_t width, const uint32_t height) {
  this->width = width;
  this->height = height;
  initWindow();
  initVulkan();

  defaultSceneSpinEnabled = sceneFile.empty();
  if (sceneFile.empty()) {
    sceneFile = "assets/models/Cube.gltf";
  }

  if (pScene) {
    if (pScene->loadFromFile(sceneFile) && !pScene->getEntities().empty()) {
      frameCameraToScene();
      setupDefaultSceneSpin();
      uploadMeshGPUResources();
      setupSceneRenderer();
    }
  }

  // Load IBL if specified
  if (!iblFile.empty() && pRenderer) {
    pRenderer->loadIBL(iblFile);
  }


  // Call custom UI builder after ImGui is initialized
  if (pCustomUIBuilder) {
    pCustomUIBuilder(*pImGuiComponentManager);
  }

  mainLoop();
}

void SauceEngineApp::setCustomUIBuilder(std::function<void(sauce::ui::ImGuiComponentManager&)> builder) {
  pCustomUIBuilder = std::move(builder);
}

void SauceEngineApp::initVulkan() {
    uint32_t glfwExtensionsCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
    pInstance = std::make_unique<sauce::Instance>(glfwExtensions, glfwExtensionsCount);

    pRenderSurface = std::make_unique<sauce::RenderSurface>(*pInstance, window);

    physicalDevice = { *pInstance };
    logicalDevice = { physicalDevice, *pRenderSurface };

    sauce::CameraCreateInfo cameraCreateInfo {
      .scrWidth = static_cast<float>(width),
      .scrHeight = static_cast<float>(height),
    };

    pScene = std::make_unique<sauce::Scene>( cameraCreateInfo );

    sauce::RendererCreateInfo rendererCreateInfo {
      .physicalDevice = physicalDevice,
      .logicalDevice = logicalDevice,
      .renderSurface = *pRenderSurface,
      .window = window,
    };

    pRenderer = std::make_unique<sauce::Renderer>(rendererCreateInfo);

    pSolver = std::make_unique<physics::XPBDSolver>();

    // Initialize ImGui
    sauce::ImGuiRendererCreateInfo imguiCreateInfo{
      .instance = **pInstance,
      .physicalDevice = physicalDevice,
      .logicalDevice = logicalDevice,
      .queueFamilyIndex = logicalDevice.getQueueIndex(),
      .window = window,
      .queue = pRenderer->getQueue(),
      .commandPool = pRenderer->getCommandPool(),
      .swapChain = pRenderer->getSwapChain(),
      .imageCount = static_cast<uint32_t>(pRenderer->getSwapChain().getImages().size()),
      .swapChainFormat = pRenderer->getSwapChain().getSurfaceFormat().format,
      .depthFormat = sauce::GraphicsPipeline::findDepthFormat(physicalDevice),
    };

    pImGuiRenderer = std::make_unique<sauce::ImGuiRenderer>(imguiCreateInfo);

    // Add default UI components
    pImGuiComponentManager->addComponent(std::make_unique<sauce::ui::HelloWorldWindow>());
    pImGuiComponentManager->addComponent(std::make_unique<sauce::ui::DebugStatsWindow>());
  }

void SauceEngineApp::initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, "Vulkan Playground", nullptr, nullptr);

    glfwSetWindowUserPointer(window, this);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  }

void SauceEngineApp::processInput(float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS && !gravePressedLastFrame) {
      cursorCaptured = !cursorCaptured;
      glfwSetInputMode(window, GLFW_CURSOR, cursorCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
      firstMouse = true;
    }
    gravePressedLastFrame = glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS;

    const bool demoTriggerPressed = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;
    if (demoTriggerPressed && !demoTriggerPressedLastFrame && pScene) {
      startDropDemo();
    }
    demoTriggerPressedLastFrame = demoTriggerPressed;

    if (!pScene) {
      return;
    }

    auto& camera = pScene->getCameraRW();
    const glm::vec3 previousCameraPosition = camera.getPos();

    if (!cursorCaptured) {
      if (cameraCollisionEnabled) {
        applyCameraCollisionPush(previousCameraPosition, deltaTime);
      }
      return;
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
      camera.processDirection(Camera::Movement::FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
      camera.processDirection(Camera::Movement::BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
      camera.processDirection(Camera::Movement::LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
      camera.processDirection(Camera::Movement::RIGHT, deltaTime);

    if (cameraCollisionEnabled) {
      applyCameraCollisionPush(previousCameraPosition, deltaTime);
    }
  }

void SauceEngineApp::updateDefaultSceneSpin(float deltaTime) {
    if (!defaultSceneSpinEnabled || !pScene || deltaTime <= 0.0f || isPhysicsDemoScene()) {
      return;
    }

    if (!defaultSceneSpinActive || defaultSceneSpinEntityName.empty()) {
      Entity* candidate = findDefaultSceneSpinEntity();
      if (!candidate) {
        defaultSceneSpinActive = false;
        defaultSceneSpinEntityName.clear();
        return;
      }
      defaultSceneSpinActive = true;
      defaultSceneSpinEntityName = candidate->get_name();
    }

    for (auto& entity : pScene->getEntitiesMut()) {
      if (entity.get_name() != defaultSceneSpinEntityName) {
        continue;
      }

      auto* transform = entity.getComponent<TransformComponent>();
      if (!transform) {
        return;
      }

      const glm::quat currentOrientation = transform->getRotation();
      const glm::quat nextOrientation = integrateAngularVelocity(
        currentOrientation,
        defaultSceneSpinAngularVelocity,
        deltaTime
      );

      transform->setRotation(nextOrientation);
      return;
    }
}

void SauceEngineApp::frameCameraToScene() {
    if (!pScene) {
      return;
    }

    SceneBounds bounds;
    for (const auto& entity : pScene->getEntities()) {
      if (!entity.getActive()) {
        continue;
      }

      const auto* meshRenderer = entity.getComponent<MeshRendererComponent>();
      const auto* transform = entity.getComponent<TransformComponent>();
      if (!meshRenderer || !meshRenderer->getMesh() || !transform) {
        continue;
      }

      expandSceneBounds(bounds, *meshRenderer->getMesh(), transform->getLocalMatrix());
    }

    if (!bounds.valid) {
      return;
    }

    const glm::vec3 center = 0.5f * (bounds.min + bounds.max);
    const glm::vec3 extents = bounds.max - bounds.min;
    float radius = 0.5f * glm::length(extents);
    if (radius < 0.5f) {
      radius = 0.5f;
    }

    auto& camera = pScene->getCameraRW();
    const float halfFovRadians = glm::radians(camera.getFOV() * 0.5f);
    const float distance = std::max(radius / std::tan(halfFovRadians), radius * 1.2f) * 1.35f;
    const glm::vec3 viewDirection = glm::normalize(glm::vec3(1.0f, 1.0f, 0.7f));
    camera.lookAt(center + viewDirection * distance, center, glm::vec3(0.0f, 0.0f, 1.0f));
}

bool SauceEngineApp::isPhysicsDemoScene() const {
    return sceneFile.find("testScene") != std::string::npos;
}

Entity* SauceEngineApp::findDefaultSceneSpinEntity() {
    if (!pScene) {
      return nullptr;
    }

    Entity* candidate = nullptr;
    size_t meshEntityCount = 0;
    for (auto& entity : pScene->getEntitiesMut()) {
      if (!entity.getActive()) {
        continue;
      }

      auto* meshRenderer = entity.getComponent<MeshRendererComponent>();
      auto* transform = entity.getComponent<TransformComponent>();
      if (!meshRenderer || !meshRenderer->getMesh() || !transform) {
        continue;
      }

      ++meshEntityCount;
      if (entity.get_name() == "Cube") {
        return &entity;
      }

      if (!candidate) {
        candidate = &entity;
      }
    }

    return meshEntityCount == 1 ? candidate : nullptr;
}

RigidBodyComponent* SauceEngineApp::ensureEntityRigidBody(Entity& entity) {
    auto* meshRenderer = entity.getComponent<MeshRendererComponent>();
    auto* transform = entity.getComponent<TransformComponent>();
    if (!meshRenderer || !meshRenderer->getMesh() || !transform) {
      return nullptr;
    }

    auto* rigidBody = entity.getComponent<RigidBodyComponent>();
    if (!rigidBody) {
      entity.addComponent<RigidBodyComponent>(
        transform->getTranslation(),
        glm::vec3(0.0f),
        transform->getRotation(),
        glm::vec3(0.0f)
      );
      rigidBody = entity.getComponent<RigidBodyComponent>();
    }

    if (!rigidBody) {
      return nullptr;
    }

    configureRigidBodyFromEntity(entity, *rigidBody);
    return rigidBody;
}

void SauceEngineApp::configureRigidBodyFromEntity(Entity& entity, RigidBodyComponent& rigidBody) {
    auto* meshRenderer = entity.getComponent<MeshRendererComponent>();
    auto* transform = entity.getComponent<TransformComponent>();
    if (!meshRenderer || !meshRenderer->getMesh() || !transform) {
      return;
    }

    rigidBody.setPosition(transform->getTranslation());
    rigidBody.setOrientation(transform->getRotation());
    rigidBody.setScale(transform->getScale());
    const glm::vec3 centerOfMass = RigidBodyComponent::meshCenterOfMass(meshRenderer->getMesh());
    rigidBody.setCenterOfMass(centerOfMass);

    float invMass = 0.0f;
    if (!isStaticTableMesh(meshRenderer)) {
      invMass = RigidBodyComponent::scaledMeshInvMass(meshRenderer->getMesh(), transform->getScale());
      if (invMass <= std::numeric_limits<float>::epsilon()) {
        invMass = 1.0f;
      }
    }
    rigidBody.setInvMass(invMass);
    rigidBody.setInvInertiaTensor(
      RigidBodyComponent::meshInvInertiaTensor(
        meshRenderer->getMesh(),
        centerOfMass,
        invMass,
        transform->getScale())
    );
}

void SauceEngineApp::applyCameraCollisionPush(const glm::vec3& previousCameraPosition, float deltaTime) {
    if (!cameraCollisionEnabled || !pScene) {
      return;
    }

    auto& camera = pScene->getCameraRW();
    const glm::vec3 cameraPosition = camera.getPos();
    const glm::vec3 cameraDisplacement = cameraPosition - previousCameraPosition;
    const float displacementLengthSq = glm::length2(cameraDisplacement);
    const float displacementLength = displacementLengthSq > 1e-10f
        ? std::sqrt(displacementLengthSq)
        : 0.0f;
    const glm::vec3 cameraMoveDirection = displacementLength > 1e-5f
        ? cameraDisplacement / displacementLength
        : glm::vec3(0.0f);
    const float cameraSpeed = deltaTime > 1e-6f ? displacementLength / deltaTime : 0.0f;

    for (auto& entity : pScene->getEntitiesMut()) {
      if (!entity.getActive()) {
        continue;
      }

      auto* meshRenderer = entity.getComponent<MeshRendererComponent>();
      auto* transform = entity.getComponent<TransformComponent>();
      if (!meshRenderer || !meshRenderer->getMesh() || !transform) {
        continue;
      }

      auto* rigidBody = entity.getComponent<RigidBodyComponent>();
      const glm::vec3 centerOfMass = rigidBody
          ? rigidBody->getCenterOfMass()
          : RigidBodyComponent::meshCenterOfMass(meshRenderer->getMesh());
      const glm::vec3 bodyScale = rigidBody ? rigidBody->getScale() : transform->getScale();
      const float bodyRadius = computeScaledMeshRadius(
          *meshRenderer->getMesh(),
          centerOfMass,
          bodyScale);
      if (bodyRadius <= 1e-5f) {
        continue;
      }

      const glm::vec3 bodyCenter = rigidBody
          ? rigidBody->getWorldCenterOfMass()
          : transform->getTranslation() + transform->getRotation() * (centerOfMass * bodyScale);
      const glm::vec3 delta = bodyCenter - cameraPosition;
      const float distanceSq = glm::length2(delta);
      const float combinedRadius = cameraCollisionRadius + bodyRadius;
      if (distanceSq >= combinedRadius * combinedRadius) {
        continue;
      }

      if (!rigidBody) {
        rigidBody = ensureEntityRigidBody(entity);
      }

      if (!rigidBody || !rigidBody->isDynamic() || !rigidBody->isCollisionEnabled()) {
        continue;
      }

      const float distance = distanceSq > 1e-10f ? std::sqrt(distanceSq) : 0.0f;
      glm::vec3 contactNormal = distance > 1e-5f ? (delta / distance) : glm::vec3(1.0f, 0.0f, 0.0f);
      if (distance <= 1e-5f && displacementLength > 1e-5f) {
        contactNormal = cameraMoveDirection;
      }

      const float penetration = combinedRadius - distance;
      rigidBody->setWorldCenterOfMass(bodyCenter + contactNormal * penetration);

      glm::vec3 velocity = rigidBody->getVelocity();
      const float inwardSpeed = glm::dot(velocity, -contactNormal);
      if (inwardSpeed > 0.0f) {
        velocity += inwardSpeed * contactNormal;
      }

      if (cameraSpeed > 0.0f) {
        const float alignedPushSpeed = std::max(glm::dot(cameraMoveDirection, contactNormal), 0.0f) * cameraSpeed;
        velocity += cameraMoveDirection * alignedPushSpeed;
      }

      velocity += contactNormal * (penetration * 8.0f);
      rigidBody->setVelocity(velocity);

      if (cameraSpeed > 0.0f) {
        rigidBody->setAngularVelocity(
            rigidBody->getAngularVelocity() +
            glm::cross(cameraMoveDirection, contactNormal) * (cameraSpeed * 0.35f));
      }
    }
}

void SauceEngineApp::startDropDemo() {
    if (!pScene) {
      return;
    }

    dropDemoActive = false;

    for (auto& entity : pScene->getEntitiesMut()) {
      if (!entity.getActive()) {
        continue;
      }

      auto* rigidBody = ensureEntityRigidBody(entity);
      auto* meshRenderer = entity.getComponent<MeshRendererComponent>();
      auto* transform = entity.getComponent<TransformComponent>();
      if (!rigidBody || !meshRenderer || !meshRenderer->getMesh()) {
        continue;
      }

      if (transform) {
        rigidBody->setPosition(transform->getTranslation());
        rigidBody->setOrientation(transform->getRotation());
      }
      rigidBody->setVelocity(glm::vec3(0.0f));
      rigidBody->setAngularVelocity(glm::vec3(0.0f));
      rigidBody->clearAccumulatedForce();
      rigidBody->setCollisionEnabled(true);

      if (isStaticTableMesh(meshRenderer)) {
        rigidBody->setInvMass(0.0f);
        rigidBody->setInvInertiaTensor(glm::mat3(0.0f));
        continue;
      }
    }

    dropDemoActive = true;
}

void SauceEngineApp::updateDropDemoForces() {
    if (!dropDemoActive || !pScene) {
      return;
    }
    constexpr glm::vec3 kGravityAcceleration(0.0f, 0.0f, -9.81f);
    constexpr float kLinearDamping = 0.25f;

    for (auto& entity : pScene->getEntitiesMut()) {
      auto* rigidBody = entity.getComponent<RigidBodyComponent>();
      if (!rigidBody || !entity.getActive()) {
        continue;
      }

      if (!rigidBody->isDynamic()) {
        rigidBody->clearAccumulatedForce();
        continue;
      }

      const glm::vec3 acceleration = kGravityAcceleration - rigidBody->getVelocity() * kLinearDamping;
      rigidBody->setAccumulatedForce(acceleration / rigidBody->getInvMass());
    }
}

void SauceEngineApp::mouseCallback(GLFWwindow* window, double xposIn, double yposIn) {
    auto* app = static_cast<SauceEngineApp*>(glfwGetWindowUserPointer(window));
    if (!app || !app->cursorCaptured) {
      return;
    }

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (app->firstMouse) {
      app->lastX = xpos;
      app->lastY = ypos;
      app->firstMouse = false;
    }

    float xoffset = xpos - app->lastX;
    float yoffset = app->lastY - ypos;

    app->lastX = xpos;
    app->lastY = ypos;

    if (!app->pScene) {
      return;
    }

    app->pScene->getCameraRW().processMouseMovement(xoffset, yoffset);
  }

void SauceEngineApp::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
      auto currentFrameTime = std::chrono::steady_clock::now();
      deltaFrame = std::chrono::duration<float>(currentFrameTime - lastFrameTime).count();
      lastFrameTime = currentFrameTime;
      deltaUpdate += deltaFrame;

      glfwPollEvents();
      processInput(deltaFrame);
      updateDefaultSceneSpin(static_cast<float>(deltaFrame));
      syncRigidBodiesToTransforms();

      pImGuiRenderer->newFrame();
      buildExampleUI();

      // Run XPBD only if 1/TICKRATE seconds passed since last physics run 

      auto constraints = std::vector<std::unique_ptr<physics::Constraint>>();
      auto rigidBodies = collectRigidBodies();

      constexpr float kPhysicsDt = 1.0f / 128.0f;
      if (deltaUpdate > 1.0) {
        deltaUpdate = kPhysicsDt;
      }
      while (deltaUpdate >= kPhysicsDt) {
        updateDropDemoForces();
        pSolver->solvePositions(rigidBodies, constraints, kPhysicsDt);
        syncRigidBodiesToTransforms();
        for (auto& entity : pScene->getEntitiesMut()) {
          if (!entity.getActive()) {
            continue;
          }

          for (auto* clothComp : entity.getComponents<ClothComponent>()) {
            if (!clothComp->hasClothData()) {
              continue;
            }

            physics::ClothData* cloth = clothComp->getClothData();
            if (cloth && !cloth->empty()) {
              pSolver->solveCloth(*cloth, kPhysicsDt);
            }
          }
        }
        deltaUpdate -= kPhysicsDt;
      }

      pRenderer->drawFrame(logicalDevice, *pScene, pImGuiRenderer.get());
    }

    logicalDevice->waitIdle();
  }

void SauceEngineApp::buildExampleUI() {
    pImGuiComponentManager->renderAll();
  }

void SauceEngineApp::syncRigidBodiesToTransforms() {
    if (!pScene) {
      return;
    }

    for (auto& entity : pScene->getEntitiesMut()) {
      auto* rigidBody = entity.getComponent<RigidBodyComponent>();
      auto* transform = entity.getComponent<TransformComponent>();
      if (!rigidBody || !transform) {
        continue;
      }

      transform->setTranslation(rigidBody->getPosition());
      transform->setRotation(rigidBody->getOrientation());
    }
  }

std::vector<RigidBodyComponent*> SauceEngineApp::collectRigidBodies() {
  std::vector<RigidBodyComponent*> rigidBodies;
  if (!pScene) {
    return rigidBodies;
  }

  auto& entities = pScene->getEntitiesMut();
  rigidBodies.reserve(entities.size());
  for (auto& entity : entities) {
    if (auto* rigidBody = entity.getComponent<RigidBodyComponent>()) {
      rigidBodies.push_back(rigidBody);
    }
  }

  return rigidBodies;
}

void SauceEngineApp::uploadMeshGPUResources() {
  for (auto& entity : pScene->getEntitiesMut()) {
    auto mrcs = entity.getComponents<MeshRendererComponent>();
    for (auto* mrc : mrcs) {
      auto mesh = mrc->getMesh();
      if (mesh && mesh->isValid() && !mesh->hasGPUData()) {
        auto& physDev = const_cast<vk::raii::PhysicalDevice&>(*physicalDevice);
        auto& cmdPool = const_cast<vk::raii::CommandPool&>(pRenderer->getCommandPool());
        auto& queue = const_cast<vk::raii::Queue&>(pRenderer->getQueue());
        mesh->initVulkanResources(logicalDevice, physDev, cmdPool, queue);
      }

      auto material = mrc->getMaterial();
      if (material && !material->hasDescriptorSet()) {
        auto& physDev = const_cast<vk::raii::PhysicalDevice&>(*physicalDevice);
        auto& cmdPool = const_cast<vk::raii::CommandPool&>(pRenderer->getCommandPool());
        auto& queue = const_cast<vk::raii::Queue&>(pRenderer->getQueue());
        material->initVulkanResources(
          logicalDevice, physDev, cmdPool, queue,
          pRenderer->getDescriptorPool(),
          pRenderer->getDefaultImageView(),
          pRenderer->getDefaultSampler()
        );
      }
    }
  }
}

void SauceEngineApp::setupSceneRenderer() {
  pRenderer->setCommandBufferRecorder(
    [this](vk::raii::CommandBuffer& cmd, uint32_t imageIndex) {
      recordSceneCommandBuffer(cmd, imageIndex);
    }
  );
}

void SauceEngineApp::setupXPBDSolver() {
  
}

void SauceEngineApp::setupDefaultSceneSpin() {
  defaultSceneSpinActive = false;
  defaultSceneSpinEntityName.clear();
}

void SauceEngineApp::recordSceneCommandBuffer(vk::raii::CommandBuffer& cmd, uint32_t imageIndex) {
  // Write camera matrices to UBO (host side, before GPU execution)
  sauce::UniformBufferObject uboData {
    .model = glm::mat4(1.0f),
    .view = pScene->getCameraRO().getViewMatrix(),
    .proj = pScene->getCameraRO().getProjectionMatrix(),
    .cameraPos = pScene->getCameraRO().getPos(),
  };
  uboData.proj[1][1] *= -1; // Vulkan Y-flip
  std::memcpy(pRenderer->getCurrentUniformBufferMapped(), &uboData, sizeof(uboData));

  auto gpuLights = pScene->collectGPULights();

  uint32_t lightCount = pRenderer->updateLightSSBO(
      gpuLights.data(), static_cast<uint32_t>(gpuLights.size()));

  cmd.begin({});

  // Transition swapchain image -> ColorAttachmentOptimal
  pRenderer->transitionImageLayout(
    cmd,
    pRenderer->getSwapChain().getImages()[imageIndex],
    vk::ImageLayout::eUndefined,
    vk::ImageLayout::eColorAttachmentOptimal,
    {},
    vk::AccessFlagBits2::eColorAttachmentWrite,
    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    vk::ImageAspectFlagBits::eColor
  );

  // Transition depth image -> DepthAttachmentOptimal
  pRenderer->transitionImageLayout(
    cmd,
    *pRenderer->getDepthImage(),
    vk::ImageLayout::eUndefined,
    vk::ImageLayout::eDepthAttachmentOptimal,
    vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
    vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
    vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
    vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
    vk::ImageAspectFlagBits::eDepth
  );

  auto& swapChain = pRenderer->getSwapChain();
  vk::Extent2D extent = swapChain.getExtent();

  vk::ClearValue clearColor = vk::ClearColorValue { 0.0f, 0.0f, 0.0f, 1.0f };
  vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);

  bool firstDraw = true;

  for (auto& entity : pScene->getEntitiesMut()) {
    if (!entity.getActive()) continue;

    auto mrcs = entity.getComponents<MeshRendererComponent>();
    if (mrcs.empty()) continue;

    // Get model matrix from transform
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    auto* tc = entity.getComponent<TransformComponent>();
    if (tc) {
      modelMatrix = tc->getLocalMatrix();
    }

    // Begin rendering (clear on first, load on subsequent)
    vk::RenderingAttachmentInfo colorAttachment {
      .imageView = swapChain.getImageViews()[imageIndex],
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = firstDraw ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = clearColor,
    };
    vk::RenderingAttachmentInfo depthAttachment {
      .imageView = pRenderer->getDepthImageView(),
      .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
      .loadOp = firstDraw ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = clearDepth,
    };
    vk::RenderingInfo renderingInfo {
      .renderArea = { .offset = { 0, 0 }, .extent = extent },
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachment,
      .pDepthAttachment = &depthAttachment,
    };

    cmd.beginRendering(renderingInfo);

    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pRenderer->getPipeline());
    cmd.setViewport(0, vk::Viewport(0.0f, 0.0f,
      static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f));
    cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), extent));
    
    // Bind Set 0: Per-frame
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
      pRenderer->getPipeline().getLayout(), 0, { *pRenderer->getCurrentDescriptorSet() }, nullptr);
    
    // Bind Set 1: Environment
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
      pRenderer->getPipeline().getLayout(), 1, { pRenderer->getEnvironmentDescriptorSet() }, nullptr);

    ScenePushConstants pushData {
      .model = modelMatrix,
      .lightCount = lightCount
    };
    cmd.pushConstants<ScenePushConstants>(
      *pRenderer->getPipeline().getLayout(),
      vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
      0u, pushData
    );

    for (auto* mrc : mrcs) {
      auto mesh = mrc->getMesh();
      auto material = mrc->getMaterial();
      if (!mesh || !mesh->hasGPUData()) continue;
      mesh->bind(cmd);

      // Bind Set 2: Material
      if (material && material->hasDescriptorSet()) {
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
          pRenderer->getPipeline().getLayout(), 2, { material->getDescriptorSet() }, nullptr);
      } else {
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
          pRenderer->getPipeline().getLayout(), 2, { pRenderer->getDefaultMaterialDescriptorSet() }, nullptr);
      }
      
      mesh->draw(cmd);
    }

    cmd.endRendering();
    firstDraw = false;
  }

  // If no entities were drawn, still clear the screen
  if (firstDraw) {
    vk::RenderingAttachmentInfo colorAttachment {
      .imageView = swapChain.getImageViews()[imageIndex],
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = clearColor,
    };
    vk::RenderingAttachmentInfo depthAttachment {
      .imageView = pRenderer->getDepthImageView(),
      .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eDontCare,
      .clearValue = clearDepth,
    };
    vk::RenderingInfo renderingInfo {
      .renderArea = { .offset = { 0, 0 }, .extent = extent },
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachment,
      .pDepthAttachment = &depthAttachment,
    };
    cmd.beginRendering(renderingInfo);
    cmd.endRendering();
  }

  // ImGui pass (load existing framebuffer contents)
  {
    vk::RenderingAttachmentInfo colorAttachment {
      .imageView = swapChain.getImageViews()[imageIndex],
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eLoad,
      .storeOp = vk::AttachmentStoreOp::eStore,
    };
    vk::RenderingAttachmentInfo depthAttachment {
      .imageView = pRenderer->getDepthImageView(),
      .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eLoad,
      .storeOp = vk::AttachmentStoreOp::eDontCare,
    };
    vk::RenderingInfo renderingInfo {
      .renderArea = { .offset = { 0, 0 }, .extent = extent },
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachment,
      .pDepthAttachment = &depthAttachment,
    };
    cmd.beginRendering(renderingInfo);
    if (pImGuiRenderer) {
      pImGuiRenderer->render(cmd, imageIndex);
    }
    cmd.endRendering();
  }

  // Transition to present
  pRenderer->transitionImageLayout(
    cmd,
    pRenderer->getSwapChain().getImages()[imageIndex],
    vk::ImageLayout::eColorAttachmentOptimal,
    vk::ImageLayout::ePresentSrcKHR,
    vk::AccessFlagBits2::eColorAttachmentWrite,
    {},
    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    vk::PipelineStageFlagBits2::eBottomOfPipe,
    vk::ImageAspectFlagBits::eColor
  );

  cmd.end();
}

} // namespace sauce
