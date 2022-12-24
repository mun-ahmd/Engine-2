#pragma once

#include <vector>
#include <unordered_map>
#include <functional>

#include "boost/hana/tuple.hpp"

#include "Utils/SparseArray.h"
#include "Utils/View.h"
#include "Pool.h"
#include "Component/Component.h"
#include "System/System.h"
#include "Utils/CompileTimeTypeHash.h"

#include "BaseComponents.h"


template <class... args>
using tuple_BH = boost::hana::tuple<args...>;

template <class arg1, class arg2>
using pair_BH = boost::hana::pair<arg1, arg2>;

class ECSmanager
{
	typedef std::unordered_map<size_t, Pool> ComponentTypeData;
private:

	ComponentTypeData component_data_map;
	uint32_t next_unused_entity_id = 0;
	
	std::vector<std::function<void(ECSmanager&)>> systems;

	template<class T>
	inline size_t get_type_id() const
	{
		return typeid(T).hash_code();
	}

	template<class ComponentType>
	inline Pool& get_pool()
	{
		return component_data_map.at(get_type_id<ComponentType>());
	}

	template<class ComponentType>
	inline const Pool& get_pool() const
	{
		static_assert(std::is_const<ComponentType>::value == true);
		return component_data_map.at(get_type_id<ComponentType>());
	}

	template<class arg1,class also>
	View<arg1> get_view_also_having()
	{
		auto view_ = get_pool<arg1>().get_entities();
		std::vector<std::pair<uint32_t, uint32_t>> start_end;
		start_end.push_back({ 0,0 });
		for (uint32_t i = 0; i < view_.size(); ++i)
		{
			if (has_component<also>(view_[i]))
			{
				if (start_end.back().first == 0)
				{
					start_end.back().second += 1;
				}
				else
				{

				}
			}
		}
	}

public:

	template <class ComponentType>
	void register_component()
	{
		component_data_map[get_type_id<ComponentType>()].template init<ComponentType>();
	}

	void register_system(std::function<void(ECSmanager&)> system_to_register)
	{
		systems.push_back(system_to_register);
	}

	ECSmanager()
	{
		//initialize any type you want to at the start here:
	}

	void execute_systems()
	{
		std::vector<std::function<void(ECSmanager&)>>::iterator system_pointer = systems.begin();
		for (; system_pointer < systems.end(); system_pointer++)
		{
			(*system_pointer)(*this);
		}
	}

	template<class... ComponentTypes>
	Entity new_entity(ComponentTypes... comps_val)
	{
		Entity new_entity = Entity(next_unused_entity_id);
		add_components(new_entity, comps_val...);
		next_unused_entity_id++;
		return new_entity;
	}



	template<class arg, class... args>
	void add_components(Entity entity, arg comp_val, args... comps_val)
	{
		((Pool&)get_pool<arg>()).add_component(entity, comp_val);
		return this->add_components(entity, comps_val...);
	}

	template<class arg>
	void add_components(Entity entity, arg final_comp) 
	{
		((Pool&)get_pool<arg>()).add_component(entity, final_comp);
		return;
	}

	template <class ComponentType>
	bool has_component(Entity entity)
	{
		get_pool<ComponentType>().has_entity(entity);
	}


	template<class ComponentType, class... ComponentTypes>
	View<ComponentType, ComponentTypes...> get_components()
	{
		
	}


	void get_components(Entity entity) {}

	template<class arg>
	arg* get_component(Entity entity){
		return (get_pool<arg>().template get_component<arg>(entity));
	}

	template<class arg, class... args>
	void get_components(Entity entity, arg* curr, args*... future)
	{
		curr = get_pool<arg>().template get_component<arg>(entity);
		get_components(entity, future...);
	}

	template<class T>
	uint32_t get_num_entities_using_component()
	{
		return	this->component_data_map[get_type_id<T>()].get_num_entities();
	}


	/*
	template<class T>
	View<T> get_components_of_type()
	{
		return this->get_pool<T>().get_components<T>();
	}

	template<class ComponentType1, class ComponentType2>
	View<ComponentType1,ComponentType2> get_components_of_type()
	{
		Pool type_pool[2] = { get_pool<ComponentType1>(), get_pool<ComponentType2>() };
		bool lower_index = (type_pool[0].get_num_entities() > type_pool[1].get_num_entities());
		auto entities = type_pool[lower_index].get_entities();
		std::vector<Entity> requested_entities;
		requested_entities.reserve(entities.size());
		for (uint32_t i = 0; i < entities.size(); ++i)
		{
			if (lower_index == 0)
			{
				if (has_component<ComponentType2>(entities[i]))
				{
					requested_entities.push_back(entities[i]);
				}
			}
			else
			{
				if (has_component<ComponentType1>(entities[i]))
				{
					requested_entities.push_back(entities[i]);
				}
			}
		}


	}
	*/
	template<class ComponentType1, class ComponentType2,class... ComponentTypes>
	View<ComponentType1, ComponentType2,ComponentTypes...> get_components_of_type()
	{

	}

	template<class ComponentType1, class ComponentType2>
	View<ComponentType1, ComponentType2> get_components_of_type()
	{

	}

	template<class ComponentType>
	View<ComponentType> get_components_of_type()
	{
		return get_pool<ComponentType>().template get_components<ComponentType>();
	}

	template<class ComponentType>
	View<ComponentType> get_components_of_type() const
	{
		static_assert(std::is_const<ComponentType>::value == true);
		return get_pool<ComponentType>().template get_components<ComponentType>();
	}

};






//TODO MAJOR
//TODO MAJOR
//TODO MAJOT
// MAKE SURE YOU MAKE AN CONSTRUCTOR FOR VIEW THAT TAKES ARRAY,LEN PAIRS SO YOU CAN GIVE THE SAME ARRAY UPTO DIFF LEVELS, 
// YOU CAN ALSO CHANGE START ALREADY BY CHANGING THE ARRAY START POINTER YOU GIVE
// THIS WILL ENABLE YOU TO DO THINGS LIKE TAKE DIFFERENT PARTS OF THE SAME ARRAY
//ALSO MAKE IT POSSIBLE TO INITIALIZE FROM CHAR*