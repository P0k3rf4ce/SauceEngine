#ifndef MODEL_PROPERTIES_HPP
#define MODEL_PROPERTIES_HPP

#include <string>
#include <memory>
#include <unordered_map>
#include <variant>

#include "animation/AnimationProperties.hpp"
#include "modeling/Camera.hpp"

namespace animation {
    class AnimationProperties;
}

namespace modeling {
    class Model;

    // Type alias for the variant that can hold different property types
    using PropertyValue = std::variant<int, bool, float, double, std::string>;

    /**
     * Stores all model related properties of an object
    */
    class ModelProperties {
    public:
        ModelProperties(std::string gltfFilename);
        ~ModelProperties();

        /**
         * This function is meant to load these 
         * Model properties back into use
        */
        void load();

        /**
         * This function is meant to remove these 
         * Model properties from use with the
         * intention that they will be used in the future.
        */
        void unload();

        /**
         * Do the various buffer setups to prepare the model
         * for the shader program and bind vertex data
        */
        void update(const animation::AnimationProperties &animProps);

		/*
		 * Set the camera used to render this model.
		 * Will take effect on the next update()
		 */
		void set_camera(std::shared_ptr<Camera> cam) {this->cam=cam;}

        // Property management methods
        template<typename T>
        void setProperty(const std::string& tag, const T& value);
        
        template<typename T>
        T getProperty(const std::string& tag) const;
        
        bool hasProperty(const std::string& tag) const;
        void removeProperty(const std::string& tag);

        // Model access
        std::shared_ptr<Model> getModel() const { return model; }
        void setModel(std::shared_ptr<Model> newModel) { model = newModel; }

    private:
        std::shared_ptr<Model> model;
        std::unordered_map<std::string, PropertyValue> properties;
        std::string gltfFilename;
		std::shared_ptr<Camera> cam;
    };

    // Template method implementations
    template<typename T>
    void ModelProperties::setProperty(const std::string& tag, const T& value) {
        properties[tag] = value;
    }

    template<typename T>
    T ModelProperties::getProperty(const std::string& tag) const {
        auto it = properties.find(tag);
        if (it != properties.end()) {
            return std::get<T>(it->second);
        }
        throw std::runtime_error("Property '" + tag + "' not found");
    }
}

#endif
