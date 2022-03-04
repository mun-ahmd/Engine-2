#include "Asset/Asset.h"
#include "Utils/SparseArray.h"
#include <atomic>
#include <forward_list>
#include <unordered_map>


static std::atomic_uint32_t asset_id_counter = 0;
class AssetManager {
private:
	SparseArray<uint32_t> id_to_index;

	SparseArray<std::string> id_to_filedir;
	//I know this isnt great having both, but time is great too.
	std::unordered_map<std::string, uint32_t> filedir_to_id;

	std::vector<std::shared_ptr<void>> active_assets;
	std::forward_list<uint32_t> free_indices;
	inline void place_active_asset(uint32_t id, std::shared_ptr<void> active_ptr) {
		if (free_indices.empty() == false) {
			id_to_index[id] = free_indices.front();
			active_assets[free_indices.front()] = active_ptr;
			free_indices.pop_front();
		}
		else {
			id_to_index.set(id,active_assets.size());
			active_assets.push_back(active_ptr);
		}
	}
	inline bool rm_active_asset(uint32_t id) {
		uint32_t rm_index = id_to_index[id];
		if (active_assets[rm_index].use_count() > 1) {
			//TODO SHOOT HEAVY WARNING
			//abort delete to avoid asset manager losing access to asset
			return false;
		}
		active_assets[rm_index].reset();
		free_indices.push_front(rm_index);
		id_to_index.rem(id);
		return true;
	}
public:
	//is this a real asset id?
	inline bool is_registered_asset(uint32_t id) { return id_to_filedir.exists(id); }
	//is this asset currently loaded?
	inline bool is_active_asset(uint32_t id) { return id_to_index.exists(id); }
	//does asset not have file associated with it?
	inline bool is_runtime_asset(uint32_t id) { return id_to_filedir.get(id).value_or(std::string()).empty(); }

	std::shared_ptr<void> get_asset(uint32_t id) {
		if (this->is_active_asset(id) == false) {
			return nullptr;
		}
		return this->active_assets[this->id_to_index[id]];
	}
	std::string get_asset_filedir(uint32_t id) {
		assert(this->id_to_filedir.exists(id));
		return this->id_to_filedir[id];
	}
	void set_active(uint32_t id, std::shared_ptr<void> active_ptr) {
		assert(is_registered_asset(id));
		assert(is_active_asset(id) == false);
		this->place_active_asset(id, active_ptr);
	}
	void new_asset(uint32_t id, std::string filedir = "") {
#ifndef NDEBUG
		if (is_registered_asset(id)) {
			//TODO shoot a warning
		}
#endif // !NDEBUG
		id_to_filedir.set(id, filedir);
		if (filedir.empty() == false) {
			filedir_to_id[filedir] = id;
		}
	}
	inline uint32_t next_id(std::string filedir) {
		if (filedir.empty() == false && filedir_to_id.find(filedir) != filedir_to_id.end()) {
			//there already exists an asset with same file
			//so just give the id of that asset
			return filedir_to_id[filedir];
		}
		else {
			//create new id because no same asset exists
			return asset_id_counter.fetch_add(1);
		}
	}
	bool deactivate_asset(uint32_t id) {
		if (is_active_asset(id) == false) {
			//TODO shoot a warning
		}
		return this->rm_active_asset(id);
	}
	void forget_asset(uint32_t id) {
		if (is_active_asset(id) == true) {
			if (deactivate_asset(id) == false) {
				//there are shared pointers to data out there
				//early return so asset is not deleted or forgotten
				return;
			}
		}
		this->id_to_filedir.rem(id);
	}
};

static AssetManager asset_map;

uint32_t __gen_eng2asset_id__(std::string filedir) {
	auto asset_id = asset_map.next_id(filedir);
	asset_map.new_asset(asset_id,filedir);
	return asset_id;
}

std::shared_ptr<void> __get_eng2asset_ptr__(uint32_t id) {
	return asset_map.get_asset(id);
}

std::string __get_eng2asset_filedir__(uint32_t id) {
	return asset_map.get_asset_filedir(id);
}

void __set_eng2asset_ptr__(uint32_t id, std::shared_ptr<void> asset_ptr) {
	//stores a shared_ptr to loaded asset, so it is not deleted unless ordered to
	asset_map.set_active(id, asset_ptr);
}

void __del_eng2asset_ptr__(uint32_t id) {
	if (asset_map.is_runtime_asset(id)) {
		//runtime assets do not have files to reload them from, so we forget them
		// not implementing recycling the ids yet though
		asset_map.forget_asset(id);
	}
	else {
		//unloads the asset for now, but remembers the filedir to load it
		asset_map.deactivate_asset(id);
	}
}

bool __check_eng2asset_valid_id__(uint32_t id) {
	return asset_map.is_registered_asset(id);
}