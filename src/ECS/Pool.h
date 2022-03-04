#pragma once

#include "Utils/ByteBuffer.h"
#include "Utils/SparseArray.h"
#include "Utils/View.h"
#include "Entity/Entity.h"


class Pool
{
	//pool just stores byte buffers
	typedef std::vector<char> ComponentData;
	typedef View<char*> PoolView;

private:
	//todo test
	SparseArray<uint32_t> _indices;
	std::vector<Entity> _entities;
	ByteBuffer _components = ByteBuffer(0);	//todo you might wanna replace vector of vectors with vector of a struct whose size cannot be changed after initialization

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
		_components = ByteBuffer(sizeof(T));
	}

	template <class T>
	void add_component(Entity entity, T obj)
	{
		assert(sizeof(T) == _components.data_type_size());	//does not make it completely safe as diff structs can have same size, 
												//but those checks will be ensured by ECSmanager anyway, so this can be removed
		assert(_components.data_type_size() != 0);
		if (entity_in_pool(entity))
		{
			uint32_t index = entity_index(entity);
			_components.set<T>(index, obj);
		}
		else
		{
			_indices.set(entity.get_id(), _entities.size());
			_entities.push_back(entity);
			_components.push<T>(obj);
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
	View<ComponentType> get_components()
	{
		return View<ComponentType>((ComponentType*)_components[0], _components.size());
	}

	template <class ComponentType>
	View<ComponentType> get_components() const
	{
		static_assert(std::is_const<ComponentType>::value == true);
		return View<ComponentType>((ComponentType*)_components[0], _components.size());
	}

	template <class ComponentType>
	ComponentType* get_component(Entity entity)
	{
		return (ComponentType*)_components[entity_index(entity)];
	}

};