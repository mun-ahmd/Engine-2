#include "SceneIO.h"
using namespace nlohmann;

#include <string>
#include <unordered_map>

std::unordered_map<size_t, std::string> type_key_name =
{
	{typeid(Position).hash_code(),"Position"}
};

void load_scene(const char* filename, ECSmanager* ecs)
{

}

void store_scene(const char* filename, const ECSmanager* ecs)
{
	json scene_j;
	scene_j["Objects"] = json::array();
	auto objects = ecs->get_components_of_type<const Position>();
	for (size_t i = 0; i < objects.size(); ++i)
	{
		json curr_obj;
		const Position& curr = objects[i];
		curr_obj[type_key_name[typeid(Position).hash_code()]] = json::array({curr.pos.x,curr.pos.y,curr.pos.z});
		scene_j["Objects"].push_back(curr_obj);
	}
	std::ofstream file(filename);
	if (!file.is_open())
	{
		std::cerr << "Could not store scene";
		exit(12);
	}
	file << json(scene_j);
	file.close();
}
