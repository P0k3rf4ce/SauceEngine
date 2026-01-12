#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <string>
#include <memory>

#include "animation/AnimationProperties.hpp"
#include "modeling/ModelProperties.hpp"
#include "rendering/RenderProperties.hpp"

#include "rendering/LightProperties.hpp"
namespace rendering { class LightProperties; }

class Object {
private:
    std::string gltfFilename;
    std::shared_ptr<animation::AnimationProperties> animProps;
    std::shared_ptr<modeling::ModelProperties> modelProps;
    std::shared_ptr<rendering::RenderProperties> renderProps;
public:
    Object();
    Object(std::shared_ptr<animation::AnimationProperties> animProps, std::shared_ptr<modeling::ModelProperties> modelProps, std::shared_ptr<rendering::RenderProperties> renderProps);
    Object(std::string gltfFilename);
    ~Object();

    void checkObjType();

    void load();
    void unload();


    void updateAnimation(double DELTA_STEP);
    void updateModeling();
    void updateRendering(bool shadow, std::shared_ptr<rendering::LightProperties> lightProps);
};

#endif
