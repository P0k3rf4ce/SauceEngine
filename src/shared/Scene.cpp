#include "shared/Scene.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <memory>
#include <utility>
#include <vector>

#include "rendering/SpotLightProperties.hpp"

// note to emmy
namespace
{
    struct SpotLightGpuData
    {
        glm::vec4 position;
        glm::vec4 direction;
        glm::vec4 color;
        glm::mat4 lightSpaceMatrix;
        glm::vec4 cutoff;
    };

    constexpr unsigned int SPOT_LIGHT_SSBO_BINDING = 2;
}

// define static active scene pointer
Scene *Scene::s_activeScene = nullptr;

Scene::Scene() {
    // set this scene as the active scene
    s_activeScene = this;
}

Scene::Scene(std::string &filename) {
    s_activeScene = this;
}

Scene::~Scene() {
    if (s_activeScene == this) {
        s_activeScene = nullptr;
    }
    if (m_spotLightSSBO != 0) {
        glDeleteBuffers(1, &m_spotLightSSBO);
        m_spotLightSSBO = 0;
    }
}

/**
 * This function is meant to load these Animation properties back into use
*/
void Scene::load() {
    for (auto object: this->objects) {
        object.load();
    }
}

/**
 * This function is meant to remove these Animation properties from use with the
 * intention that they will be used in the future.
*/
void Scene::unload() {
    for (auto object: this->objects) {
        object.unload();
    }
}

/**
 * Update the Animation properties <timestep> seconds into the future
*/
double Scene::update(double deltatime, double DELTA_STEP) {
    while (deltatime >= DELTA_STEP) {
        for (auto object: this->objects) {
            object.updateAnimation(DELTA_STEP);
        }
        deltatime -= DELTA_STEP;
    }

    for (auto object: this->objects) {
        object.updateModeling();
    }

    for (auto object: this->objects) {
        object.updateRendering();
    }

    return deltatime;
}

const std::vector<std::shared_ptr<rendering::LightProperties>> &Scene::getLights() const noexcept {
    return lights;
}

Scene *Scene::getActiveScene() noexcept {
    return s_activeScene;
}

void Scene::addLight(std::shared_ptr<rendering::LightProperties> light) {
    lights.push_back(std::move(light));
}

// self-note to emmy here
void Scene::draw(rendering::Shader& shader) {
    uploadSpotLightsBuffer();
    for (auto object: this->objects) {
        object.draw(shader);
    }
}

// note to emmy
void Scene::uploadSpotLightsBuffer() {
    if (m_spotLightSSBO == 0) {
        glGenBuffers(1, &m_spotLightSSBO);
    }

    std::vector<SpotLightGpuData> gpuLights;
    for (const auto &light : lights) {
        auto spot = std::dynamic_pointer_cast<rendering::SpotLightProperties>(light);
        if (!spot) {
            continue;
        }

        SpotLightGpuData gpuLight{};
        gpuLight.position = glm::vec4(spot->getPosition(), 1.0f);
        gpuLight.direction = glm::vec4(glm::normalize(spot->getDirection()), 0.0f);
        gpuLight.color = glm::vec4(spot->getColour(), 1.0f);
        gpuLight.lightSpaceMatrix = spot->getLightSpaceMatrix();
        gpuLight.cutoff = glm::vec4(spot->getCutOff(), spot->getOuterCutOff(), 0.0f, 0.0f);
        gpuLights.push_back(gpuLight);
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_spotLightSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 static_cast<GLsizeiptr>(gpuLights.size() * sizeof(SpotLightGpuData)),
                 gpuLights.empty() ? nullptr : gpuLights.data(),
                 GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SPOT_LIGHT_SSBO_BINDING, m_spotLightSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}
