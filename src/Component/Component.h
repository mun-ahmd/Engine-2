#pragma once

class Entity;
//this forward declaration for entity, because Entity.h includes this file

class Component
{
public:
	virtual bool isEnabled() = 0;
	virtual void enable() = 0;
	virtual void disable() = 0;
	virtual void getType() = 0;
	virtual void start(Entity* entity);
};