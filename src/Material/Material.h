#pragma once
#include "Graphics/Graphics_2.h"
#include <array>
#include "Asset/TextureAsset.h"


class PBR_Material
{
	//make sure all textures are made resident when the material is loaded
private:
	// while I can group these to be 2 4 channel and 1 3 channel texture(s)
	//  but since I am using bindless textures, this works and it saves memory too

	Texture2DAsset albedo;		//3 channel
	Texture2DAsset roughness;	//1 channel
	Texture2DAsset metalness;	//1 channel
	Texture2DAsset alpha;		//1 channel
	Texture2DAsset ambient_occlusion;	//1 channel
	Texture2DAsset emission;	//1 channel
	Texture2DAsset normal;		//3 channel
	//total size of handles array = 8 * 7 = 56 bytes (not a multiple of 16 too lol)
	
public:
	

	PBR_Material(Texture2DAsset albedo, Texture2DAsset roughness, Texture2DAsset metalness,Texture2DAsset alpha, Texture2DAsset ambient_occlusion, Texture2DAsset emission, Texture2DAsset normal)
		: albedo(albedo), roughness(roughness), metalness(metalness), alpha(alpha), ambient_occlusion(ambient_occlusion), emission(emission), normal(normal) {}

	inline std::array<uint64_t, 7> get_handles	()
	{
		return std::array<uint64_t, 7>({ albedo.get_asset()->get_handle(), roughness.get_asset()->get_handle() ,metalness.get_asset()->get_handle(),
											alpha.get_asset()->get_handle(),ambient_occlusion.get_asset()->get_handle(), emission.get_asset()->get_handle(), normal.get_asset()->get_handle() });
	}
	void make_resident()
	{
		albedo.get_asset()->make_handle_resident();
		roughness.get_asset()->make_handle_resident();
		metalness.get_asset()->make_handle_resident();
		alpha.get_asset()->make_handle_resident();
		ambient_occlusion.get_asset()->make_handle_resident();
		emission.get_asset()->make_handle_resident();
		normal.get_asset()->make_handle_resident();

	}

};

typedef PBR_Material Material;