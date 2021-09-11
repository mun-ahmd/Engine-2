#pragma once

#include "Utils/View.h"
#include "boost/hana/tuple.hpp"
#include <functional>
#include <tuple>

template<class ComponentType, class... ComponentTypes>
class System
{
private:
	uint16_t min_elements_per_thread = 1;
	std::function<void(ComponentType, ComponentTypes...)> first_call_func = [](ComponentType, ComponentTypes...) {};
	std::function<void(ComponentType, ComponentTypes...)> call_func;
public:
	System(std::function<void(ComponentType,ComponentTypes...)> given_call_func)
	{
		call_func = given_call_func;
	}
	void call(boost::hana::tuple<View<ComponentType,ComponentTypes...>> views)
	{
	}
	void call(View<ComponentType> view)
	{
		for (uint32_t i = 0; i < view.size(); ++i)
		{
			call_func(view[i]);
		}
	}
};