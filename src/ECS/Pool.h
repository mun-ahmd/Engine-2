#pragma once

#include "Utils/ByteBuffer.h"
#include "Utils/SparseArray.h"
#include "Utils/View.h"
#include "Entity/Entity.h"
#include <vector>
#include <type_traits>

template<typename T>
class PoolVector{
	public:
	inline static std::vector<typename std::remove_const<T>::type> components;
};

class Pool
{
	//pool just stores byte buffers
	typedef std::vector<char> ComponentData;
	typedef View<char*> PoolView;

private:
	//todo test
	SparseArray<uint32_t> _indices;
	std::vector<Entity> _entities;
	// ByteBuffer PoolVector<T>::components = ByteBuffer(0);
	size_t component_size;

	bool entity_in_pool(Entity entity)
	{
		if (_indices.exists(entity.get_id()))
			return true;
		return false;
	}
	inline size_t entity_index(Entity entity)
	{
		assert(entity_in_pool(entity) == true);
		return _indices[entity.get_id()];
	}

public:

	template <class T>
	void init()
	{
		static_assert(std::is_const<T>::value == false);
		component_size = sizeof(T);
	}

	template <class T>
	void add_component(Entity entity, T obj)
	{
		assert(sizeof(T) == component_size);	//does not make it completely safe as diff structs can have same size, 
												//but those checks will be ensured by ECSmanager anyway, so this can be removed
		assert(component_size != 0);
		if (entity_in_pool(entity))
		{
			uint32_t index = entity_index(entity);
			PoolVector<T>::components[index] = obj;
		}
		else
		{
			_indices.set(entity.get_id(), _entities.size());
			_entities.push_back(entity);
			PoolVector<T>::components.push_back(obj);
		}
	}

	bool has_entity(Entity entity)
	{
		if (_indices.has_val(entity_index(entity)))
			return true;
		return false;
	}

	uint32_t get_num_entities()
	{
		return this->_entities.size();
	}

	const View<Entity> get_entities()
	{
		return View<Entity>(&_entities);
	}

	template <class ComponentType>
	View<typename std::remove_const<ComponentType>::type> get_components()
	{
		return View<typename std::remove_const<ComponentType>::type>(PoolVector<ComponentType>::components.data(), PoolVector<ComponentType>::components.size());
	}

	template <class ComponentType>
	View<const typename std::remove_const<ComponentType>::type> get_components() const
	{
		return View<const typename std::remove_const<ComponentType>::type>((const typename std::remove_const<ComponentType>::type*)PoolVector<ComponentType>::components.data(), PoolVector<ComponentType>::components.size());
	}

	template <class ComponentType>
	typename std::remove_const<ComponentType>::type* get_component(Entity entity)
	{
		return &PoolVector<ComponentType>::components[entity_index(entity)];
	}

};