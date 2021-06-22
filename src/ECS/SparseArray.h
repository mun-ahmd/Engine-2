#pragma once
#include <assert.h>
#include <vector>

//todo test

template<typename T>
class OptEle
{
private:
	bool is_value;
	T value;
public:
	OptEle()
	{
		is_value = false;
		value = NULL;
	}
	bool has_value()
	{
		return is_value;
	}
	void set(T val_in)
	{
		value = val_in;
		is_value = true;
	}
	void rem_val()
	{
		value = NULL;
		is_value = false;
	}
	T& val()
	{
		assert(is_value = true);
		return value;
	}
};

template <typename T>
class SparseArray
{
private:
	std::vector<OptEle<T>> arr;
	//PackedArray<size_t> destroyed;
public:
	void set(size_t index, T item)
	{
		if (index >= arr.size())
		{
			arr.resize(index + 1);
		}
		arr[index].set(item);
	}

	void rem(size_t index)
	{
		assert(index < arr.size());
		arr[index].rem_val();
	}

};