#ifndef SCENE_HPP
#define SCENE_HPP

#include <string>
#include <vector>

#include "shared/Object.hpp"

class Scene {
private:
    std::vector<Object> objects;
public:
    Scene();
    Scene(std::string &filename);
    ~Scene();

    void load();
    void unload();
    void update(double timestep);
};

#endif