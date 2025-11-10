#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <string>
#include <memory>

#include "animation/AnimationProperties.hpp"
#include "modeling/ModelProperties.hpp"
#include "rendering/RenderProperties.hpp"

namespace modeling{
    class ModelingProperties;
}


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

    void load();
    void unload();


	/*
	 * Set the camera used by modelProps for rendering the object.
	 * Will take effect on the next update()
	 */
	inline void set_camera(std::shared_ptr<Camera> cam) {
		this->modelProps->set_camera(cam);
	}

    void updateAnimation(double DELTA_STEP);
    void updateModeling();
    void updateRendering();
};

#endif
