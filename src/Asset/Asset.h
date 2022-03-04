#pragma once

#include <memory>
#include <vector>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <assert.h>

struct AssetFileLoc {
	std::string original_file;
	std::string asset_file;
};

//template<class AssetType>
//using AssetLoader = std::shared_ptr<AssetType>(*)(std::string filepath);
//
//template<class AssetType>
//using AssetStorer = void(*)(std::shared_ptr<const AssetType> asset_item, std::string filepath);



std::shared_ptr<void> __get_eng2asset_ptr__(uint32_t id);
std::string __get_eng2asset_filedir__(uint32_t id);
void __set_eng2asset_ptr__(uint32_t id, std::shared_ptr<void> asset_ptr);
void __del_eng2asset_ptr__(uint32_t id);
uint32_t __gen_eng2asset_id__(std::string filedir = "");
bool __check_eng2asset_valid_id__(uint32_t id);


template<class AssetType>
inline void DefaultUnloadFunction(std::shared_ptr<AssetType> asset_item) {
	//do nothing.
	//This function exists to be able to check if an asset is non-storable
}

//TODO TODO TODO
// REPLACE TAKING FUNCTIONS TEMPLATE WITH A WAY OF STORING STD FUNCTION FOR EVERY CLASS THANKS
//TODO TODO TODO


template<
	class AssetType,
	std::shared_ptr<AssetType>(*asset_loader)(std::string),
	void(*asset_unloader)(std::shared_ptr<AssetType> asset_item) = DefaultUnloadFunction
>
class Asset
{
private:
	uint32_t asset_id;
	inline std::shared_ptr<AssetType> load_asset(const char* filedir) {
		std::shared_ptr<AssetType> loaded_ = asset_loader(filedir);
		__set_eng2asset_ptr__(this->asset_id, loaded_);
		return loaded_;
	}
public:
	Asset() {
		//for debug only this constructor
		this->asset_id = __gen_eng2asset_id__();
	}
	Asset(const char* filedir) {
		//no need to load here, will be loaded from file on get
		this->asset_id = __gen_eng2asset_id__(filedir);
	}
	Asset(uint32_t id) : asset_id(id) {
		assert(__check_eng2asset_valid_id__(id));
	}
	Asset(std::unique_ptr<AssetType> held_data) {
		this->asset_id = __gen_eng2asset_id__();
		__set_eng2asset_ptr__(this->asset_id,std::move(held_data));	
	}

	std::shared_ptr<AssetType> get_asset() {
		std::shared_ptr<AssetType> required_ptr = std::static_pointer_cast<AssetType>(__get_eng2asset_ptr__(this->asset_id));
		if (required_ptr == nullptr) {
			required_ptr = load_asset(__get_eng2asset_filedir__(this->asset_id).c_str());
		}
		return required_ptr;
	}

	void unload() {
		// not reloadable if it is a runtime asset
		asset_unloader(this->get_asset());
		__del_eng2asset_ptr__(this->asset_id);
	}
};

//TODO PLEASE TEST IT WELL

