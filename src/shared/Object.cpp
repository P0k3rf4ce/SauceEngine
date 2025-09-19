#include "shared/Object.hpp"

Object::Object(std::string gltfFilename) {
    this->modelProps = std::shared_ptr<modeling::ModelProperties>(new modeling::ModelProperties(gltfFilename));
    this->animProps = std::shared_ptr<animation::AnimationProperties>(new animation::AnimationProperties(*(this->modelProps.get())));
    this->renderProps = std::shared_ptr<rendering::RenderProperties>(new rendering::RenderProperties(*(this->modelProps.get())));
}

Object::~Object() {
    this->renderProps.reset();
    this->animProps.reset();
    this->modelProps.reset();
}

/**
 * This function is meant to load this Object back into use
*/
void Object::load() {
    this->modelProps->load();
    this->animProps->load();
    this->renderProps->load();
}

/**
 * This function is meant to remove this Object from use with the
 * intention that they will be used in the future.
*/
void Object::unload() {
    this->renderProps->unload();
    this->animProps->unload();
    this->modelProps->unload();
}

/**
 * Update the Object <timestep> seconds into the future
*/
void Object::update(double timestep) {
    this->animProps->update(timestep);
    this->modelProps->update(*(this->animProps.get()));
    this->renderProps->update(*(this->modelProps.get()), *(this->animProps.get()));
}