#pragma once
#include "Graphics/Graphics_2.h"
#include <array>


class PBR_Material
{
	//make sure all textures are made resident when the material is loaded
private:
	// while I can group these to be 2 4 channel and 1 3 channel texture(s)
	//  but since I am using bindless textures, this works and it saves memory too

	Texture_2D albedo;		//3 channel
	Texture_2D roughness;	//1 channel
	Texture_2D metalness;	//1 channel
	Texture_2D alpha;		//1 channel
	Texture_2D ambient_occlusion;	//1 channel
	Texture_2D emission;	//1 channel
	Texture_2D normal;		//3 channel
	//total size of handles array = 8 * 7 = 56 bytes (not a multiple of 16 too lol)
	
public:
	

	PBR_Material(Texture_2D albedo, Texture_2D roughness, Texture_2D metalness,Texture_2D alpha, Texture_2D ambient_occlusion, Texture_2D emission, Texture_2D normal)
		: albedo(albedo), roughness(roughness), metalness(metalness), alpha(alpha), ambient_occlusion(ambient_occlusion), emission(emission), normal(normal) {}

	inline std::array<uint64_t, 7> get_handles	()
	{
		return std::array<uint64_t, 7>({ albedo.get_handle(), roughness.get_handle() ,metalness.get_handle(),
											alpha.get_handle(),ambient_occlusion.get_handle(), emission.get_handle(), normal.get_handle() });
	}
	void make_resident()
	{
		albedo.make_handle_resident();
		roughness.make_handle_resident();
		metalness.make_handle_resident();
		alpha.make_handle_resident();
		ambient_occlusion.make_handle_resident();
		emission.make_handle_resident();
		normal.make_handle_resident();

	}

};

typedef PBR_Material Material;