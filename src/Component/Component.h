#pragma once
#include <vector>

class Entity;
//this forward declaration for entity

enum ComponentTypes_ENG2
{
	//never use = for any element, it will make NUM_OF_COMPONENT_TYPES wrong
	COMPONENT_CAMERA,
	COMPONENT_MESH,
	COMPONENT_MESH_RENDERER,

	NUM_OF_COMPONENT_TYPES
};

class Component
{
public:
	virtual bool isEnabled() = 0;
	virtual void enable() = 0;
	virtual void disable() = 0;
	virtual std::vector<ComponentTypes_ENG2> getType() = 0;
	virtual void start(Entity* entity);
};

/*
class ComponentData
{
	std::vector<Component*> data;
public:
	ComponentData()
	{
		data.reserve((int)(NUM_OF_COMPONENT_TYPES));
		for (int i = 0; i < NUM_OF_COMPONENT_TYPES; ++i)
		{
			data.push_back(nullptr);
		}
	}
	Component* operator [](uint32_t index)
	{
		//assert(index < NUM_OF_COMPONENT_TYPES && index >= 0);		//don't need cause this assert is in vector too
		//returns nullptr if its not there
		return(data[index]);
	}
	const Component* operator [](uint32_t index) const
	{
		return(data[index]);
	}
	bool has_component(ComponentTypes_ENG2 type)
	{
		if (data[(int)(type)] == nullptr)
			return false;
		return true;
	}
};
*/