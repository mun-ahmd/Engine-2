//file including or defining base component types
#pragma once
#include "Mesh/Mesh.h"
#include "Material/Material.h"

struct Position
{
	Position(float val = 1.0f)
	{
		pos = glm::vec3(val);
	}
	glm::vec3 pos;
};