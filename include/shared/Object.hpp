#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <string>

#ifdef __linux__
    #include <memory>
#endif

#include "animation/AnimationProperties.hpp"
#include "modeling/ModelProperties.hpp"
#include "rendering/RenderProperties.hpp"

class Object {
private:
    std::string gltfFilename;
    std::shared_ptr<animation::AnimationProperties> animProps;
    std::shared_ptr<modeling::ModelProperties> modelProps;
    std::shared_ptr<rendering::RenderProperties> renderProps;
public:
    Object(std::string gltfFilename);
    ~Object();

    void load();
    void unload();

    void updateAnimation();
    void updateModeling();
    void updateRendering();
};

#endif