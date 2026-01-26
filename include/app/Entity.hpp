#pragma once

#include <vector>
#include <memory>

#include <app/Component.hpp>


namespace sauce {

class Entity {
public:
	Entity(const std::string& name) : name(name) {}

	std::string get_name() const { return name; }
	bool getActive() const { return active; }
	void setActive(bool active);


	/**
	 * Initializes and adds a component of type <T> with name <name>
	 *
	 * @param name Name for the new component
	 */
	template <typename T, typename... Args>
	void addComponent(Args &&...args) {
		// TODO
	}

	/**
	 * Removes the most recently added component of a specified type
	 */
	template <typename T>
	void removeComponent() {
		// TODO
	}

	/**
	 * Removes the most recently added component of a specified type and name
	 *
	 * @param name Name of the component to remove
	 */
  template <typename T>
	void removeComponent(const std::string& name) {
		// TODO
	}

	/**
	 * Returns a raw pointer to the most recently added component of a specified type
	 */
	template <typename T>
	T* getComponent() {
		// TODO
	}

	/*
	 * Returns a raw pointer to the most recently added component of a specified type and name
	 *
	 * @param name Name of the component to find
	 */
	template <typename T>
	T* getComponent(const std::string& name) {

  }

private:
	std::string name;
	bool active = true;
	std::vector<std::unique_ptr<Component>> components;
};

}
