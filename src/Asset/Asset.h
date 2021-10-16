#pragma once

#include <string>
#include <filesystem>
#include "Model.h"



template<class type>
class Asset
{
private:
	std::string original_file;
	std::string asset_file;
	type asset;
	template<class load_type>
	void load_internal()
	{
		std::cout << "no load method defined for type held by asset";
	}
	template<>
	void load_internal<Model>()
	{
		if (std::filesystem::exists(asset_file))
		{
			asset.loadModel(asset_file.c_str(), false, false, false);
		}
		else
		{
			asset.loadModel(original_file.c_str(), false, true, false);
		}
	}

public:	
	type* load(std::string filepath,std::string asset_dir)
	{
		original_file = filepath;
		asset_file = asset_dir;
		load_internal<type>();
		return asset.get();
	}

	inline type* get_asset()
	{
		return &asset;
	}
};

