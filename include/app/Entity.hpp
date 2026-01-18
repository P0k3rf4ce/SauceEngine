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
	template <typename T>
	void add_component(const std::string name);

	template <typename T>
	void remove_component();

	void remove_component(const std::string name);

	template <typename T>
	T* get_component();

	template <typename T>
	T* get_component(const std::string name);

private:
	std::string name;
	bool active;
	std::vector<std::unique_ptr<Component>> components;
};

}
