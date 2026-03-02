#pragma once

#include <vector>
#include <memory>
#include <iterator>
#include <utility>

#include <app/Component.hpp>


namespace sauce {

class Entity {
public:
	Entity(const std::string& name) : name(name) {}

	std::string get_name() const { return name; }
	void set_name(const std::string& newName) { name = newName; }
	bool getActive() const { return active; }
	void setActive(bool active) { this->active = active; }


	/**
	 * Initializes and adds a component of type <T> with name <name>
	 *
	 * @param name Name for the new component
	 */
	template <typename T, typename... Args>
	void addComponent(Args &&...args) {
		auto component = std::make_unique<T>(std::forward<Args>(args)...);
		component->setOwner(this);
		components.push_back(std::move(component));
	}

	/**
	 * Removes the most recently added component of a specified type
	 */
	template <typename T>
	void removeComponent() {
		for (auto it = components.rbegin(); it != components.rend(); ++it) {
			if (dynamic_cast<T*>(it->get()) != nullptr) {
				components.erase(std::next(it).base());
				return;
			}
		}
	}

	/**
	 * Removes the most recently added component of a specified type and name
	 *
	 * @param name Name of the component to remove
	 */
  template <typename T>
	void removeComponent(const std::string& name) {
		for (auto it = components.rbegin(); it != components.rend(); ++it) {
			auto* component = dynamic_cast<T*>(it->get());
			if (component != nullptr && component->name == name) {
				components.erase(std::next(it).base());
				return;
			}
		}
	}

	/**
	 * Removes a specific component by pointer
	 */
	void removeComponentByPointer(Component* target) {
		for (auto it = components.begin(); it != components.end(); ++it) {
			if (it->get() == target) {
				components.erase(it);
				return;
			}
		}
	}

	/**
	 * Returns a raw pointer to the most recently added component of a specified type
	 */
	template <typename T>
	T* getComponent() {
		for (auto it = components.rbegin(); it != components.rend(); ++it) {
			auto* component = dynamic_cast<T*>(it->get());
			if (component != nullptr) {
				return component;
			}
		}
		return nullptr;
	}

	template <typename T>
	const T* getComponent() const {
		for (auto it = components.rbegin(); it != components.rend(); ++it) {
			auto* component = dynamic_cast<const T*>(it->get());
			if (component != nullptr) {
				return component;
			}
		}
		return nullptr;
	}

	/**
	 * Returns raw pointers to all components of a specified type
	 */
	template <typename T>
	std::vector<T*> getComponents() {
		std::vector<T*> result;
		for (auto& comp : components) {
			auto* c = dynamic_cast<T*>(comp.get());
			if (c) result.push_back(c);
		}
		return result;
	}

	template <typename T>
	std::vector<const T*> getComponents() const {
		std::vector<const T*> result;
		for (const auto& comp : components) {
			auto* c = dynamic_cast<const T*>(comp.get());
			if (c) result.push_back(c);
		}
		return result;
	}

	/*
	 * Returns a raw pointer to the most recently added component of a specified type and name
	 *
	 * @param name Name of the component to find
	 */
	template <typename T>
	T* getComponent(const std::string& name) {
		for (auto it = components.rbegin(); it != components.rend(); ++it) {
			auto* component = dynamic_cast<T*>(it->get());
			if (component != nullptr && component->name == name) {
				return component;
			}
		}
		return nullptr;
  }

private:
	std::string name;
	bool active = true;
	std::vector<std::unique_ptr<Component>> components;
};

}
