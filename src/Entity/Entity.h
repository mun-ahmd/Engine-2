#pragma once
#include <vector>
#include "glm/glm.hpp"

#include "Component/Component.h"
//Note: Entity is forward declared in Component.h

class Entity
{
public:
	std::vector<Component*> components;
};