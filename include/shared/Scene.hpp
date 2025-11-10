#ifndef SCENE_HPP
#define SCENE_HPP

#include <string>
#include <vector>

#include "shared/Object.hpp"
#include "utils/Camera.hpp"

class Scene {
private:
    std::vector<Object> objects;
    std::shared_ptr<Camera> active_camera = nullptr;

public:
    Scene();
    Scene(std::string &filename);
    ~Scene();

    void load();
    void unload();
    void update(double timestep);

    std::shared_ptr<Camera> get_camera();
    void set_camera(std::shared_ptr<Camera> cam);
};

#endif