#pragma once
#include <cstdint>

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
	Entity(){
		//set as invalid id with invalid version
		id_version = ENTITY_INVALID;
	}
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