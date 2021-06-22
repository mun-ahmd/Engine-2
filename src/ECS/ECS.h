#pragma once
#include "SparseArray.h"
#include <vector>
#include "Component/Component.h"
#include <unordered_map>
#include "View.h"


#define ENTITY_VERSION_NUM_BITS 4
constexpr uint32_t ENTITY_INVALID_VERSION = 0x0000000F;
constexpr uint32_t ENTITY_INVALID_ID = 0xFFFFFFF0;
constexpr uint32_t ENTITY_INVALID = ENTITY_INVALID_ID | ENTITY_INVALID_VERSION;

//below class will replace the one in entity.h
//Entity is supposed to be one 32 bit value which stores id in first y bits and the version in the least significant x bits (x = ENTITY_VERSION_NUM_BITS)
class Entity
{
private:
	//version to take 4 bits, id to take rest
	uint32_t id_version;
public:
	Entity(uint32_t id)
	{
		if (id > ENTITY_INVALID_ID)
			id = ENTITY_INVALID_ID;
		id_version = (id << ENTITY_VERSION_NUM_BITS);	//start at version 0
	}
	bool version_valid()
	{
		return ((id_version & ENTITY_INVALID_VERSION) != ENTITY_INVALID_VERSION);
	}
	bool id_valid()
	{
		return ((id_version & ENTITY_INVALID_ID) != ENTITY_INVALID_ID);
	}
	uint32_t get_full_id()
	{
		return id_version;
	}
	uint32_t get_version()
	{
		return (id_version & ENTITY_INVALID_VERSION);
	}
	uint32_t get_id()
	{
		return (id_version & ENTITY_INVALID_ID) >> ENTITY_VERSION_NUM_BITS;
	}
	//returns sucess of operation
	//need to figure out best way to handle invalid versions
	bool bump_version()
	{
		uint32_t old_version = id_version;
		id_version = (id_version & ENTITY_INVALID_ID) | ((get_version() + 1) & ENTITY_INVALID_VERSION);
		if (!version_valid())
		{
			id_version = old_version;
			return false;
		}
		return true;
	}
	//no bump_id function to be created
};


enum ComponentTypes_ENG2
{
	//never use = for any element, it will make NUM_OF_COMPONENT_TYPES wrong
	FIRST_COMPONENT_TYPE_THROWAWAY,
	COMPONENT_CAMERA,
	COMPONENT_MESH,
	COMPONENT_MESH_RENDERER,

	NUM_OF_COMPONENT_TYPES
};


class Pool
{
typedef std::vector<Component> ComponentData;

private:
	//todo test
	SparseArray<uint32_t> _indices;
	std::vector<Entity> entity_ids;
	std::vector<ComponentData> components;
public:
	Pool()
	{

	}
};



struct EntityComponentDataWrapper
{
	typedef std::vector<Component> ComponentData;
	Entity entity;
	ComponentData component_data;
};

class ECSmanager
{
typedef std::unordered_map<ComponentTypes_ENG2, Pool> ComponentTypeData;
private:
	ComponentTypeData pool_map;
public:
	ECSmanager()
	{
		int curr = FIRST_COMPONENT_TYPE_THROWAWAY + 1;
		while (curr < NUM_OF_COMPONENT_TYPES)
		{
			pool_map[(ComponentTypes_ENG2)curr] = Pool();
			curr++;
		}
	}
	View<EntityComponentDataWrapper,1> getEntitiesWithComponent(ComponentTypes_ENG2 type)
	{
		//todo

	}
	template <uint8_t num_types>
	View<Component, num_types> getEntitiesWithAllOfComponents(ComponentTypes_ENG2* types)
	{
		//todo
	}

};