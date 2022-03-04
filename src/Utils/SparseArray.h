#pragma once
#include <assert.h>
#include <vector>
#include <optional>

//todo test

//template<typename T>
//class OptEle
//{
//private:
//	bool is_value;
//	T value;
//public:
//	OptEle()
//	{
//		is_value = false;
//		value = NULL;
//	}
//	bool has_value()
//	{
//		return is_value;
//	}
//	void set(T val_in)
//	{
//		value = val_in;
//		is_value = true;
//	}
//	void rem_val()
//	{
//		value = NULL;
//		is_value = false;
//	}
//	T& val()
//	{
//		assert(is_value = true);
//		return value;
//	}
//};

template <typename T>
class SparseArray
{
private:
	std::vector<std::optional<T>> arr;
	//std::vector<size_t> destroyed;
public:
	std::optional<T>& get(size_t index)
	{
		return arr[index];
	}

	const std::optional<T>& get(size_t index) const
	{
		return arr[index];
	}

	bool has_val(size_t index)
	{
		return arr[index].has_value();
	}

	T& operator [](size_t index)
	{
		assert(arr[index].has_value());
		return arr[index].value();
	}

	const T& operator [](size_t index) const
	{
		assert(arr[index].has_value());
		return arr[index].value();
	}

	void set(size_t index, T item)
	{
		if (index >= arr.size())
		{
			arr.resize(index + 1);
		}
		arr[index] = item;
	}

	void rem(size_t index)
	{
		assert(index < arr.size());
		arr[index].reset();
	}

	bool exists(size_t index)
	{
		if (index >= arr.size())
			return false;
		return arr[index].has_value();
	}

};