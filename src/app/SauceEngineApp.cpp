#include <app/SauceEngineApp.hpp>
#include <app/ui/components/HelloWorldWindow.hpp>
#include <app/ui/components/DebugStatsWindow.hpp>
#include <app/components/TransformComponent.hpp>
#include <app/components/RigidBodyComponent.hpp>
#include <app/components/MeshRendererComponent.hpp>
#include <app/components/ClothComponent.hpp>
#include <app/components/LightComponent.hpp>
#include <app/PhysicsDemoSetup.hpp>
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

struct Ray {
  glm::vec3 origin;
  glm::vec3 direction;
};

struct AABB {
  glm::vec3 min;
  glm::vec3 max;
};

Ray screenToWorldRay(const Camera& camera, double mouseX, double mouseY,
                     float viewportWidth, float viewportHeight) {
  float ndcX = (2.0f * static_cast<float>(mouseX)) / viewportWidth - 1.0f;
  float ndcY = 1.0f - (2.0f * static_cast<float>(mouseY)) / viewportHeight;

  glm::mat4 proj = camera.getProjectionMatrix();
  glm::mat4 view = camera.getViewMatrix();
  glm::mat4 invProjView = glm::inverse(proj * view);

  glm::vec4 nearPoint = invProjView * glm::vec4(ndcX, ndcY, 0.0f, 1.0f);
  glm::vec4 farPoint  = invProjView * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
  nearPoint /= nearPoint.w;
  farPoint  /= farPoint.w;

  return { glm::vec3(nearPoint), glm::normalize(glm::vec3(farPoint) - glm::vec3(nearPoint)) };
}

bool rayIntersectsAABB(const Ray& ray, const AABB& box, float& tOut) {
  float tmin = 0.0f;
  float tmax = std::numeric_limits<float>::max();
  for (int i = 0; i < 3; ++i) {
    if (std::abs(ray.direction[i]) < 1e-8f) {
      if (ray.origin[i] < box.min[i] || ray.origin[i] > box.max[i])
        return false;
    } else {
      float invD = 1.0f / ray.direction[i];
      float t1 = (box.min[i] - ray.origin[i]) * invD;
      float t2 = (box.max[i] - ray.origin[i]) * invD;
      if (t1 > t2) std::swap(t1, t2);
      tmin = std::max(tmin, t1);
      tmax = std::min(tmax, t2);
      if (tmin > tmax) return false;
    }
  }
  tOut = tmin;
  return true;
}

AABB computeWorldAABB(const modeling::Mesh& mesh, const glm::mat4& modelMatrix) {
  AABB result;
  result.min = glm::vec3(std::numeric_limits<float>::max());
  result.max = glm::vec3(std::numeric_limits<float>::lowest());
  for (const auto& v : mesh.getVertices()) {
    glm::vec3 wp = glm::vec3(modelMatrix * glm::vec4(v.position, 1.0f));
    result.min = glm::min(result.min, wp);
    result.max = glm::max(result.max, wp);
  }
  return result;
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
    setupXPBDSolver();

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
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
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

    if (cameraCollisionCooldown > 0.0f) {
      cameraCollisionCooldown = std::max(0.0f, cameraCollisionCooldown - deltaTime);
    }

    auto& camera = pScene->getCameraRW();
    const glm::vec3 previousCameraPosition = camera.getPos();
    const bool cameraCollisionActive = cameraCollisionEnabled && cameraCollisionCooldown <= 0.0f;

    if (!cursorCaptured) {
      if (cameraCollisionActive) {
        applyCameraCollisionPush(previousCameraPosition, deltaTime);
      }
      return;
    }

    const bool sprintHeld =
        glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    const float baseMovementSpeed = camera.getMovementSpeed();
    camera.setMovementSpeed(sprintHeld ? baseMovementSpeed * 3.0f : baseMovementSpeed);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
      camera.processDirection(Camera::Movement::FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
      camera.processDirection(Camera::Movement::BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
      camera.processDirection(Camera::Movement::LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
      camera.processDirection(Camera::Movement::RIGHT, deltaTime);

    camera.setMovementSpeed(baseMovementSpeed);

    if (cameraCollisionActive) {
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
    return physics_demo::ensureEntityRigidBody(entity);
}

void SauceEngineApp::configureRigidBodyFromEntity(Entity& entity, RigidBodyComponent& rigidBody) {
    physics_demo::configureRigidBodyFromEntity(entity, rigidBody);
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
      if (!rigidBody && !dropDemoActive) {
        continue;
      }

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

      if (!rigidBody || !rigidBody->isCollisionEnabled()) {
        continue;
      }

      const float distance = distanceSq > 1e-10f ? std::sqrt(distanceSq) : 0.0f;
      glm::vec3 contactNormal = distance > 1e-5f ? (delta / distance) : glm::vec3(1.0f, 0.0f, 0.0f);
      if (distance <= 1e-5f && displacementLength > 1e-5f) {
        contactNormal = cameraMoveDirection;
      }

      const float penetration = combinedRadius - distance;
      if (rigidBody->isSleeping()) {
        rigidBody->wake();
      }
      if (!rigidBody->isDynamic()) {
        continue;
      }

      constexpr float kPenetrationCorrectionFraction = 0.2f;
      constexpr float kMaxPenetrationCorrection = 0.03f;
      constexpr float kAlignedPushScale = 0.35f;
      constexpr float kMaxAlignedPushSpeed = 0.5f;
      constexpr float kNormalPushScale = 1.2f;
      constexpr float kMaxNormalPushSpeed = 0.2f;
      constexpr float kAngularPushScale = 0.02f;

      const float correctionDistance = std::min(
          penetration * kPenetrationCorrectionFraction,
          kMaxPenetrationCorrection);
      rigidBody->setWorldCenterOfMass(bodyCenter + contactNormal * correctionDistance);

      glm::vec3 velocity = rigidBody->getVelocity();
      const float inwardSpeed = glm::dot(velocity, -contactNormal);
      if (inwardSpeed > 0.0f) {
        velocity += inwardSpeed * contactNormal;
      }

      if (cameraSpeed > 0.0f) {
        const float alignedComponent = std::max(glm::dot(cameraMoveDirection, contactNormal), 0.0f);
        const float alignedPushSpeed = std::min(
            alignedComponent * cameraSpeed * kAlignedPushScale,
            kMaxAlignedPushSpeed);
        velocity += cameraMoveDirection * alignedPushSpeed;
      }

      velocity += contactNormal * std::min(correctionDistance * kNormalPushScale, kMaxNormalPushSpeed);
      rigidBody->setVelocity(velocity);

      if (cameraSpeed > 0.0f) {
        rigidBody->setAngularVelocity(
            rigidBody->getAngularVelocity() +
            glm::cross(cameraMoveDirection, contactNormal) * (cameraSpeed * kAngularPushScale));
      }
    }
}

void SauceEngineApp::wakeContactNeighbors(Entity& source, RigidBodyComponent& sourceBody) {
    if (!pScene) {
      return;
    }

    const glm::vec3 sourceCenter = sourceBody.getWorldCenterOfMass();
    const auto* sourceMesh = source.getComponent<MeshRendererComponent>();
    if (!sourceMesh || !sourceMesh->getMesh()) {
      return;
    }

    const float sourceRadius = computeScaledMeshRadius(
        *sourceMesh->getMesh(), sourceBody.getCenterOfMass(), sourceBody.getScale());
    constexpr float kNeighborMargin = 0.15f;

    for (auto& entity : pScene->getEntitiesMut()) {
      if (!entity.getActive() || &entity == &source) {
        continue;
      }

      auto* rigidBody = entity.getComponent<RigidBodyComponent>();
      if (!rigidBody || !rigidBody->isSleeping() || !rigidBody->canBeDynamic()) {
        continue;
      }

      auto* meshRenderer = entity.getComponent<MeshRendererComponent>();
      if (!meshRenderer || !meshRenderer->getMesh()) {
        continue;
      }

      const float neighborRadius = computeScaledMeshRadius(
          *meshRenderer->getMesh(), rigidBody->getCenterOfMass(), rigidBody->getScale());
      const glm::vec3 neighborCenter = rigidBody->getWorldCenterOfMass();
      const float dist = glm::length(neighborCenter - sourceCenter);
      const float touchDist = sourceRadius + neighborRadius + kNeighborMargin;

      if (dist < touchDist) {
        rigidBody->wake();
      }
    }
}

void SauceEngineApp::startDropDemo() {
    if (!pScene) {
      return;
    }

    dropDemoActive = false;
    physics_demo::armScene(*pScene);
    dropDemoActive = true;
    cameraCollisionCooldown = 0.2f;
}

void SauceEngineApp::updateDropDemoForces() {
    if (!dropDemoActive || !pScene) {
      return;
    }

    physics_demo::applyForces(*pScene);
  }

void SauceEngineApp::mouseCallback(GLFWwindow* window, double xposIn, double yposIn) {
    auto* app = static_cast<SauceEngineApp*>(glfwGetWindowUserPointer(window));
    if (!app) {
      return;
    }

    if (!app->cursorCaptured) {
      if (app->draggedEntity) {
        app->updateDrag(xposIn, yposIn);
      }
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

void SauceEngineApp::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    auto* app = static_cast<SauceEngineApp*>(glfwGetWindowUserPointer(window));
    if (!app || app->cursorCaptured) {
      return;
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
      if (action == GLFW_PRESS) {
        if (ImGui::GetIO().WantCaptureMouse) {
          return;
        }
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        app->beginDrag(mx, my);
      } else if (action == GLFW_RELEASE) {
        app->endDrag();
      }
    }
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
      const size_t dynamicRigidBodyCount = static_cast<size_t>(std::count_if(
          rigidBodies.begin(),
          rigidBodies.end(),
          [](const RigidBodyComponent* rigidBody) {
            return rigidBody && rigidBody->canBeDynamic() && rigidBody->isCollisionEnabled();
          }));
      const auto solverTuning = physics_demo::selectRigidSolverTuning(dynamicRigidBodyCount);
      pSolver->solverIterations = solverTuning.solverIterations;
      pSolver->rigidSubsteps = solverTuning.rigidSubsteps;
      const float physicsDt = solverTuning.physicsDt;

      if (deltaUpdate > 1.0) {
        deltaUpdate = physicsDt;
      }
      while (deltaUpdate >= physicsDt) {
        updateDropDemoForces();

        if (draggedEntity) {
          auto* dragRB = draggedEntity->getComponent<RigidBodyComponent>();
          if (dragRB) {
            glm::vec3 toTarget = dragTargetPosition - dragRB->getPosition();
            constexpr float kMaxDragStep = 0.2f;
            float maxDisp = kMaxDragStep * static_cast<float>(pSolver->rigidSubsteps);
            float dist = glm::length(toTarget);
            if (dist > maxDisp) {
              toTarget *= maxDisp / dist;
            }
            dragRB->setVelocity(toTarget / physicsDt);
            dragRB->setAngularVelocity(glm::vec3(0.0f));
          }
        }

        pSolver->solvePositions(rigidBodies, constraints, physicsDt);

        if (draggedEntity) {
          auto* dragRB = draggedEntity->getComponent<RigidBodyComponent>();
          if (dragRB) {
            dragRB->setVelocity(glm::vec3(0.0f));
            dragRB->setAngularVelocity(glm::vec3(0.0f));
          }
        }

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
              pSolver->solveCloth(*cloth, physicsDt);
            }
          }
        }
        deltaUpdate -= physicsDt;
      }

      pRenderer->drawFrame(logicalDevice, *pScene, pImGuiRenderer.get());
    }

    logicalDevice->waitIdle();
  }

void SauceEngineApp::buildExampleUI() {
    pImGuiComponentManager->renderAll();
  }

void SauceEngineApp::syncRigidBodiesToTransforms() {
    if (pScene) {
      physics_demo::syncRigidBodiesToTransforms(*pScene);
    }
  }

std::vector<RigidBodyComponent*> SauceEngineApp::collectRigidBodies() {
  return pScene ? physics_demo::collectRigidBodies(*pScene) : std::vector<RigidBodyComponent*>{};
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
  if (!pSolver) {
    return;
  }

  pSolver->solverIterations = 40;
  pSolver->rigidSubsteps = 8;
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

  // Pass 0: Clear and Render Skybox
  {
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
    cmd.setViewport(0, vk::Viewport(0.0f, 0.0f,
      static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f));
    cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), extent));
    
    pRenderer->renderEnvironmentMap(cmd);
    
    cmd.endRendering();
  }

  bool firstDraw = false; // We already cleared in Pass 0

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

void SauceEngineApp::beginDrag(double mouseX, double mouseY) {
    if (!pScene) { return; }

    const auto& camera = pScene->getCameraRW();
    const Ray ray = screenToWorldRay(camera, mouseX, mouseY,
                                     static_cast<float>(width), static_cast<float>(height));

    Entity* bestEntity = nullptr;
    float bestT = std::numeric_limits<float>::max();

    for (auto& entity : pScene->getEntitiesMut()) {
      if (!entity.getActive()) { continue; }
      auto* meshRenderer = entity.getComponent<MeshRendererComponent>();
      auto* transform = entity.getComponent<TransformComponent>();
      auto* rigidBody = entity.getComponent<RigidBodyComponent>();
      if (!meshRenderer || !meshRenderer->getMesh() || !transform || !rigidBody) { continue; }
      if (!rigidBody->canBeDynamic()) { continue; }

      const AABB box = computeWorldAABB(*meshRenderer->getMesh(), transform->getLocalMatrix());
      float t = 0.0f;
      if (rayIntersectsAABB(ray, box, t) && t < bestT) {
        bestT = t;
        bestEntity = &entity;
      }
    }

    if (!bestEntity) { return; }

    auto* rigidBody = bestEntity->getComponent<RigidBodyComponent>();

    glm::mat4 view = camera.getViewMatrix();
    dragPlaneNormal = -glm::vec3(view[0][2], view[1][2], view[2][2]);
    dragPlanePoint = rigidBody->getPosition();

    float denom = glm::dot(ray.direction, dragPlaneNormal);
    if (std::abs(denom) < 1e-8f) { return; }
    float tPlane = glm::dot(dragPlanePoint - ray.origin, dragPlaneNormal) / denom;
    glm::vec3 hitOnPlane = ray.origin + tPlane * ray.direction;

    dragOffset = rigidBody->getPosition() - hitOnPlane;
    dragPrevPosition = rigidBody->getPosition();
    dragTargetPosition = rigidBody->getPosition();
    dragSmoothedVelocity = glm::vec3(0.0f);
    draggedEntity = bestEntity;

    if (rigidBody->isSleeping()) {
      rigidBody->wake();
    }
    rigidBody->setInvMass(0.0f);
    rigidBody->setInvInertiaTensor(glm::mat3(0.0f));
    rigidBody->setVelocity(glm::vec3(0.0f));
    rigidBody->setAngularVelocity(glm::vec3(0.0f));
  }

void SauceEngineApp::updateDrag(double mouseX, double mouseY) {
    if (!draggedEntity || !pScene) { return; }

    auto* rigidBody = draggedEntity->getComponent<RigidBodyComponent>();
    if (!rigidBody) {
      draggedEntity = nullptr;
      return;
    }

    const auto& camera = pScene->getCameraRW();
    const Ray ray = screenToWorldRay(camera, mouseX, mouseY,
                                     static_cast<float>(width), static_cast<float>(height));

    float denom = glm::dot(ray.direction, dragPlaneNormal);
    if (std::abs(denom) < 1e-8f) { return; }
    float tPlane = glm::dot(dragPlanePoint - ray.origin, dragPlaneNormal) / denom;
    glm::vec3 hitOnPlane = ray.origin + tPlane * ray.direction;

    glm::vec3 newPos = hitOnPlane + dragOffset;

    const float dt = std::max(static_cast<float>(deltaFrame), 1e-4f);
    const glm::vec3 frameVel = (newPos - dragTargetPosition) / dt;
    constexpr float kSmoothFactor = 0.3f;
    dragSmoothedVelocity = glm::mix(dragSmoothedVelocity, frameVel, kSmoothFactor);

    dragTargetPosition = newPos;
  }

void SauceEngineApp::endDrag() {
    if (!draggedEntity) { return; }

    auto* rigidBody = draggedEntity->getComponent<RigidBodyComponent>();
    if (rigidBody) {
      constexpr float kMaxThrowSpeed = 4.0f;
      glm::vec3 throwVel = dragSmoothedVelocity;
      float speed = glm::length(throwVel);
      if (speed > kMaxThrowSpeed) {
        throwVel *= kMaxThrowSpeed / speed;
      }
      rigidBody->sleep();
      rigidBody->wake();
      rigidBody->setVelocity(throwVel);
    }

    draggedEntity = nullptr;
  }

} // namespace sauce
