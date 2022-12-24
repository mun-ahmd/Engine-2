#pragma once
#include <cassert>
#include <cstdint>
#include <vector>
#include <string.h>

class ByteBuffer
{
private:
	std::vector<char> buffer;
	uint16_t obj_size;
public:
	ByteBuffer(uint16_t per_obj_size)
	{
		obj_size = per_obj_size;
	}
	//no need for this function, just assign new bytebuffer of diff size
	//void set_obj_size(uint16_t per_obj_size)
	//{
	//	buffer.clear();
	//	buffer.shrink_to_fit();
	//	obj_size = per_obj_size;
	//}
	size_t size() const
	{
		return buffer.size() / obj_size;
	}
	size_t buffer_size()
	{
		return buffer.size();
	}
	uint16_t data_type_size()
	{
		return obj_size;
	}
	template<class T>
	void push(T obj)
	{
		assert(sizeof(T) == obj_size);
		buffer.resize(buffer.size() + obj_size);
		memcpy(&buffer[buffer.size() - obj_size], &obj, obj_size);
	}
	template<class T>
	void set(size_t index, T val)
	{
		assert(sizeof(T) == obj_size);
		assert(index * obj_size < buffer.size());
		memcpy(buffer.data() + index * obj_size, &val, obj_size);
	}
	char* operator [](size_t index)
	{
		return (&buffer[index * obj_size]);
	}
	const char* operator [](size_t index) const
	{
		return (&buffer[index * obj_size]);
	}
};