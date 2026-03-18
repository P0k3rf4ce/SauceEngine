#pragma once

#include <string>

namespace sauce {

    class Entity;

    class Component {
      public:
        const std::string name;

        Component(std::string n = "Component") : name(n) {
        }
        virtual ~Component() {
        }

        virtual bool getActive() {
            return this->active;
        }
        virtual void setActive(bool newState) {
            this->active = newState;
        }

        virtual void setOwner(Entity* newOwner) {
            this->owner = newOwner;
        }
        virtual Entity* getOwner() {
            return this->owner;
        }

        virtual void update(float deltaT) {};
        virtual void render() {};

      private:
        sauce::Entity* owner = nullptr;
        bool active;
    };

} // namespace sauce
