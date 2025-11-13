#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <string>
#include <memory>

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

    /**
     * This function is meant to load this Object back into use
    */
    void load();

    /**
     * This function is meant to remove this Object from use with the
     * intention that they will be used in the future.
    */
    void unload();

    /**
     * Update the Object <timestep> seconds into the future
    */
    void update(double timestep);

    /**
     * Draw the object using the given shader
    */
    void draw(rendering::Shader& shader);
};

#endif