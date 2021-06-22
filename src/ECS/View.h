#pragma once
#include <assert.h>
#include <vector>
#include <array>
#include <utility>
#include <iostream>

template<class T,uint8_t num_arrays>
class View
{
	typedef std::pair<T*, size_t> view_pair;
private:
	//max number of arrays in a view = 256
	view_pair arrays[num_arrays];
	size_t num_elements = 0;
public:
	View(std::vector<T>** arraysIN)
	{
		for (uint8_t i = 0; i < num_arrays; ++i)
		{
			arrays[i] = view_pair(arraysIN[i]->data(), arraysIN[i]->size());
			num_elements += arrays[i].second;
		}
	}
	const T& operator [](size_t index)
	{
		assert(index < num_elements);
		uint8_t arr_index = 0;
		size_t accumallated_index = arrays[arr_index].second;
		while (index > accumallated_index)
		{
			arr_index += 1;
			accumallated_index += arrays[arr_index].second;
		}
		return arrays[arr_index].first[arrays[arr_index].second - (accumallated_index - index)];
	}
	void iterate(void (*action)(const T&))
	{
		for (uint8_t arr_i = 0; arr_i < num_arrays; ++arr_i)
		{
			for (size_t i = 0; i < arrays[arr_i].second; ++i)
			{
				action(arrays[arr_i].first[i]);
			}
		}
	}
	size_t size()
	{
		return num_elements;
	}
};



template <class T>
class View<T, 1>
{
private:
	//max number of arrays in a view = 256
	std::vector<T>* array_;
public:
	View(std::vector<T>** arraysIN)
	{
			array_ = arraysIN[0];
	}
	const T& operator [](size_t index)
	{
		assert(index < array_->size());
		return ((*array_)[index]);
	}
	void iterate(void (*action)(const T&))
	{
		for (size_t i = 0; i < array_->size(); ++i)
		{
			action((*array_)[i]);
		}
	}
	size_t size()
	{
		return array_->size();
	}
};