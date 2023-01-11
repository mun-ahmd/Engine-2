#pragma once
#include <assert.h>
#include <vector>
#include <array>
#include <utility>
#include <iostream>
#include <functional>
#include "boost/hana/tuple.hpp"
#include <tuple>


//NEVER STORE A VIEW
//I am aware I can use vector pointers in the general template too, but I really want views to be a point in time, not a thing valid throughout

template<typename same, typename first, typename...more>
struct any_is_same {
	static const bool value = std::is_same<same, first>::value ||
		any_is_same<first, more...>::value;
};

template<typename same, typename first>
struct any_is_same<same, first> : std::is_same<same, first> {};

template<typename ...T, size_t... I>
inline auto makeReferencesHelper(std::tuple<T...>& t, std::index_sequence<I...>)
{
	return std::tie(*std::get<I>(t)...);
}

template<typename ...T>
inline auto makeReferences(std::tuple<T...>& t) {
	return makeReferencesHelper<T...>(t, std::make_index_sequence<sizeof...(T)>{});
}

template<class... Types>
class View
{
	template<class T>
	using view_pair = std::pair<T*, uint32_t>;
private:
	std::tuple< std::vector<view_pair<Types>>... > tupled_arrays;
	uint16_t tuple_size;
	uint32_t array_size;


	size_t index_for_general_iterate = 0;
	std::tuple<Types*...> container_for_general_iterate;
	//this member saves single threaded performance and implementation, but it forbids multiple threads iterating same view
	//that is fine because it is not intended anyway and that would also cause a data race
	//for multithreading it is much preferred that a single view is split into however many and they are used per thread

	template<std::size_t I = 0, typename... Types_>
	inline typename std::enable_if<I == sizeof...(Types_), void>::type
		iterate_tuple(std::tuple<Types_...>& t)
	{ }

	template<std::size_t I = 0, typename... Types_>
	inline typename std::enable_if < I < sizeof...(Types_), void>::type
		iterate_tuple(std::tuple<Types_...>& t)
	{
		using argType = std::tuple_element<I, std::tuple<Types_...>>;
		std::get<I>(container_for_general_iterate) = this->get_ptr<I>(index_for_general_iterate);

		iterate_tuple<I + 1, Types_...>(t);
	}


public:

	View(std::vector<std::pair<Types*, uint32_t>>...  values)
	{
		tupled_arrays = std::make_tuple(values...);
		array_size = 0;
		auto array_0 = std::get<0>(tupled_arrays);
		for (size_t i = 0; i < array_0.size(); ++i)
			array_size += array_0[i].second;
		
	}

	template<size_t index_>
	typename std::tuple_element<index_,std::tuple<Types...>>::type* get_ptr(size_t index)
	{
		assert(index < array_size);
		auto arrays = std::get<index_>(tupled_arrays);
		int i = 0;
		size_t accum = 0;
		while (arrays[i].second <= index - accum)
		{
			accum += arrays[i].second;
			i++;
		}
		// i = 2, accum = 34, index = 23
		return &((arrays[i].first[index - accum]));
	}

	template<class gType>
	gType& get(size_t index)
	{
		assert(index < array_size);
#ifndef NDEBUG
		if constexpr (any_is_same<gType, Types...>::value == false)
			throw(false);
#endif
		std::vector<view_pair<gType>>& arrays = std::get<std::vector<view_pair<gType>> >(tupled_arrays);
		int i = 0;
		size_t accum = 0;
		while(arrays[i].second <= index - accum)
		{
			accum += arrays[i].second;
			i++;
		}
		// i = 2, accum = 34, index = 23
		return ((arrays[i].first[index - accum])); 
	}

	template<class gType>
	const gType& get(size_t index) const
	{
		assert(index < array_size);
#ifndef NDEBUG
		if constexpr (any_is_same<gType, Types...>::value == false)
			throw(false);
#endif
		std::vector<view_pair<gType>>& arrays = std::get<std::vector<view_pair<gType>> >(tupled_arrays);
		int i = 0;
		size_t accum = 0;
		while (arrays[i].second <= index - accum)
		{
			accum += arrays[i].second;
			i++;
		}
		// i = 2, accum = 34, index = 23
		return ((arrays[i].first[index - accum]));
	}

	template<class iType>
	void iterate(std::function<void(iType&)> action)
	{
#ifndef NDEBUG
		if constexpr(any_is_same<iType,Types...>::value == false)
			throw(false);
#endif
		std::vector<view_pair<iType>>& arrays = std::get<std::vector<view_pair<iType>> >(tupled_arrays);
		uint16_t num_arrays = arrays.size();
		for (uint16_t arr_i = 0; arr_i < num_arrays; ++arr_i)
		{
			for (size_t i = 0; i < arrays[arr_i].second; ++i)
			{
				action(arrays[arr_i].first[i]);
			}
		}
	}

	void iterate(std::function<void(Types&...)> action)
	{
		
		for (uint32_t i = 0; i < array_size; ++i)
		{
			index_for_general_iterate = i;
			this->iterate_tuple(tupled_arrays);
			std::apply(action, makeReferences(this->container_for_general_iterate));
		}
	}

	std::tuple<Types&...> operator[](size_t index)
	{
		index_for_general_iterate = index;
		this->iterate_tuple(tupled_arrays);
		return makeReferences(this->container_for_general_iterate);
	}

	size_t size()
	{
		return array_size;
	}
};


template<class Type>
class View<Type>
{
private:
	Type* array_ptr;
	uint32_t array_size;
public:
	View(std::vector<Type>* vec) : array_ptr(vec->data()), array_size(vec->size()) {}
	View(Type* array_ptr, uint32_t array_size) : array_ptr(array_ptr), array_size(array_size) {}
	size_t size()
	{
		return array_size;
	}
	Type& operator[](uint32_t index)
	{
		assert(index < array_size);
		return array_ptr[index];
	}
	void iterate(std::function<void(Type&)> action)
	{
		for (uint32_t i = 0;i < array_size; ++i)
		{
			action(array_ptr[i]);
		}
	}
	inline Type* get_array_ptr() { return array_ptr; }
};


//
//template<class T,uint8_t num_arrays>
//class View
//{
//	typedef std::pair<T*, size_t> view_pair;
//private:
//	//max number of arrays in a view = 256
//	view_pair arrays[num_arrays];
//	size_t num_elements = 0;
//public:
//	View(std::vector<T>** arraysIN)
//	{
//		for (uint8_t i = 0; i < num_arrays; ++i)
//		{
//			arrays[i] = view_pair(arraysIN[i]->data(), arraysIN[i]->size());
//			num_elements += arrays[i].second;
//		}
//	}
//
//	T& operator [](size_t index)
//	{
//		assert(index < num_elements);
//		uint8_t arr_index = 0;
//		size_t accumallated_index = arrays[arr_index].second;
//		while (index > accumallated_index)
//		{
//			arr_index += 1;
//			accumallated_index += arrays[arr_index].second;
//		}
//		return arrays[arr_index].first[arrays[arr_index].second - (accumallated_index - index)];
//	}
//	const T& operator [](size_t index) const
//	{
//		assert(index < num_elements);
//		uint8_t arr_index = 0;
//		size_t accumallated_index = arrays[arr_index].second;
//		while (index > accumallated_index)
//		{
//			arr_index += 1;
//			accumallated_index += arrays[arr_index].second;
//		}
//		return arrays[arr_index].first[arrays[arr_index].second - (accumallated_index - index)];
//	}
//	void iterate(std::function<void(T)> action)
//	{
//		for (uint8_t arr_i = 0; arr_i < num_arrays; ++arr_i)
//		{
//			for (size_t i = 0; i < arrays[arr_i].second; ++i)
//			{
//				action(arrays[arr_i].first[i]);
//			}
//		}
//	}
//	size_t size()
//	{
//		return num_elements;
//	}
//};
//
//template <class T>
//class View<T, 1>
//{
//private:
//	//max number of arrays in a view = 256
//	T* array_;
//	size_t len;
//public:
//	View(std::vector<T>* arraysIN)
//	{
//			array_ = (*arraysIN).data();
//			len = arraysIN->size();
//	}
//	View(T* start, size_t lenIN)
//	{
//		array_ = start;
//		len = lenIN;
//	}
//	T& operator [](size_t index)
//	{
//		assert(index < len);
//		return array_[index];
//	}
//	const T& operator [](size_t index) const
//	{
//		assert(index < len);
//		return (array_[index]);
//	}
//	void iterate(std::function<void(T&)> action)
//	{
//		for (size_t i = 0; i < len; ++i)
//		{
//			action(array_[i]);
//		}
//	}
//	size_t size()
//	{
//		return len;
//	}
//};
//
////Dynamic num array view
//constexpr uint8_t DYNAMIC_VIEW = std::numeric_limits<uint8_t>::max();
//
//template <class T>
//class View<T, DYNAMIC_VIEW>
//{
//typedef std::pair<T*, size_t> view_pair;
//private:
//
//	uint8_t num_arrays;
//	std::vector<view_pair> arrays;
//	size_t num_elements = 0;
//
//public:
//
//	View(std::vector<T>** arraysIN,uint8_t num_arraysIN)
//	{
//		this->num_arrays = num_arraysIN;
//		arrays.reserve(num_arrays);
//		for (uint8_t i = 0; i < num_arrays; ++i)
//		{
//			arrays.push_back(view_pair(arraysIN[i]->data(), arraysIN[i]->size()));
//			num_elements += arrays[i].second;
//		}
//	}
//	
//	template<class other>
//	View(std::vector<T>** arraysIN,uint8_t num_arraysIN, View<other, DYNAMIC_VIEW>& other_view)
//	{
//		this->num_arrays = other_view.num_arrays;
//		assert(num_arrays == num_arraysIN);
//		arrays.reserve(num_arrays);
//		for (uint8_t i = 0; i < num_arrays; ++i)
//		{
//			assert(other_view.arrays[i].second <= arraysIN[i]->size());
//			this->arrays.push_back(view_pair(arraysIN[i]->data(), other_view.arrays[i].second));
//			num_elements += arrays[i].second();
//		}
//	}
//
//	T& operator [](size_t index)
//	{
//		assert(index < num_elements);
//		uint8_t arr_index = 0;
//		size_t accumallated_index = arrays[arr_index].second;
//		while (index > accumallated_index)
//		{
//			arr_index += 1;
//			accumallated_index += arrays[arr_index].second;
//		}
//		return arrays[arr_index].first[arrays[arr_index].second - (accumallated_index - index)];
//	}
//
//	const T& operator [](size_t index) const
//	{
//		assert(index < num_elements);
//		uint8_t arr_index = 0;
//		size_t accumallated_index = arrays[arr_index].second;
//		while (index > accumallated_index)
//		{
//			arr_index += 1;
//			accumallated_index += arrays[arr_index].second;
//		}
//		return arrays[arr_index].first[arrays[arr_index].second - (accumallated_index - index)];
//	}
//
//	void iterate(std::function<void(T&)> action)
//	{
//		for (uint8_t arr_i = 0; arr_i < num_arrays; ++arr_i)
//		{
//			for (size_t i = 0; i < arrays[arr_i].second; ++i)
//			{
//				action(arrays[arr_i].first[i]);
//			}
//		}
//	}
//
//	size_t size()
//	{
//		return num_elements;
//	}
//};



