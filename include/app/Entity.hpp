#pragma once

#include <iostream>
#include <memory>

#include <app/Component.hpp>


namespace sauce {

class Component;

class Entity {
public:
	Entity(const std::string name) : name(name) {}

	std::string get_name() { return name; }
	bool get_active() { return active; }
	void set_active(bool active);


	// TODO - TASK: implement the methods below

	/**
	 * Initializes and adds a component of type <T> with name <name>
	 *
	 * @param name Name for the new component
	 */
	template <typename T>
	void add_component(const std::string name);

	/**
	 * Removes the most recently added component of a specified type
	 */
	template <typename T>
	void remove_component();

	/**
	 * Removes the most recently added component of a specified name
	 *
	 * @param name Name of the component to remove
	 */
	void remove_component(const std::string name);

	/**
	 * Returns a raw pointer to the most recently added component of a specified type
	 */
	template <typename T>
	T* get_component();

	/*
	 * Returns a raw pointer to the most recently added component of a specified name
	 *
	 * @param name Name of the component to find
	 */
	template <typename T>
	T* get_component(const std::string name);

private:
	std::string name;
	bool active;
	std::vector<std::unique_ptr<Component>> components;
};

}
