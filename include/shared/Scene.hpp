#ifndef SCENE_HPP
#define SCENE_HPP

#include <string>
#include <vector>
#include <memory>

#include "shared/Object.hpp"
#include "utils/Camera.hpp"

namespace rendering { class LightProperties; }

struct Skybox {
	unsigned int irradiance;
	unsigned int prefilter;
	unsigned int brdf;
};

class Scene {
private:
    std::vector<Object> objects;
    std::shared_ptr<Camera> active_camera = nullptr;
    std::vector<std::shared_ptr<rendering::LightProperties>> lights;

    void uploadSpotLightsBuffer();
    unsigned int m_spotLightSSBO = 0;

    static std::shared_ptr<Scene> active_scene;

public:
    Scene();
    Scene(std::string &filename);
    ~Scene();

    std::vector<std::shared_ptr<rendering::LightProperties>> &getLights();
    void addLight(std::shared_ptr<rendering::LightProperties> light);

    void load();
    void unload();

    std::shared_ptr<Camera> get_camera();
    void set_camera(std::shared_ptr<Camera> cam);
	void renderObjects(bool shadow = false, std::shared_ptr<rendering::LightProperties> lightProps = nullptr);
    double update(double deltatime, double DELTA_STEP);

    static std::shared_ptr<Scene> getActiveScene();
    static void set_active_scene(std::shared_ptr<Scene> s);

    static unsigned int scr_width;
    static unsigned int scr_height;

	Skybox skybox;
};

#endif
