#pragma once
#include "json.hpp"
#include "ECS/ECS.hpp"

void load_scene(const char* filename, ECSmanager* ecs);
void store_scene(const char* filename, const ECSmanager* ecs);