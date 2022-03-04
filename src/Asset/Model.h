#pragma once

//Deprecated
/*
#include "Mesh/Mesh.h"
#include "Material/Material.h"


#include "Utils/BinaryFileUtility.h"

#include <vector>
#include <fstream>
#include <string>

struct MeshDataAny {
	VertexDataAny vertices;
	std::vector<unsigned int> indices;
};

struct Loaded3DSceneData {
	std::vector<MeshDataAny> meshes;
	std::vector<unsigned short> mesh_mat_id;

	std::vector<PBR_Material> materials;
	std::vector<Texture_2D> textures;
};
std::vector<MeshDataAny> load_meshes();


//model is intended to be an imported asset, from model you can choose meshes as mesh components and materials for the renderer
class Model
{
public:
	//static bool are_same_verts(Model& a, Model& b)
	//{
	//	if (a.meshes.size() != b.meshes.size())
	//		return false;
	//	for (size_t i = 0; i < a.meshes.size(); ++i)
	//	{
	//		VertexData ad = a.meshes[i]->getMeshData();
	//		VertexData bd = b.meshes[i]->getMeshData();
	//		if (ad.verts_data_size != bd.verts_data_size)
	//			return false;
	//		for (size_t i = 0; i < ad.verts_data_size; i++)
	//		{
	//			if (ad.verts_data[i] != bd.verts_data[i])
	//			{
	//				std::cout << ad.verts_data[i] << "   " << bd.verts_data[i] << '\n';
	//				//return false;
	//			}
	//		}
	//	}
	//	return true;
	//}
		
	void loadModel(const char* filedirIN,bool loadTangents, bool optimizeMesh,bool smoothNormals = false);

	void storeModel(std::string fileDirIN,std::string filename)
	{
		BinaryFileUtility file;
		auto fileDir = fileDirIN + R"(\)" + filename + ".twomodel";
		file.open(fileDir.c_str());
		int num_meshes = meshes.size();
		file.write(num_meshes);
		for (size_t i = 0; i < meshes.size(); i++)
		{
			VertexData mesh_data = meshes[i]->getMeshData();
			uint32_t indices_size = mesh_data.indices_data_size;
			file.write(indices_size);
			file.write(mesh_data.indices_data, indices_size);

			uint32_t verts_size = mesh_data.verts_data_size;
			file.write(verts_size);
			file.write(mesh_data.verts_data, verts_size );
		}
		file.close();
	}
	std::vector<Mesh*> get_meshes()
	{
		return meshes;
	}
	std::vector<Material*> get_materials()
	{
		return materials;
	}
	std::vector<std::pair<Mesh*, Material*>> get_meshes_materials()
	{
		std::vector<std::pair<Mesh*, Material*>> meshes_materials;
		meshes_materials.reserve(meshes.size());
		for (int i = 0; i < meshes.size(); ++i)
		{
			meshes_materials.push_back(std::pair(meshes[i], materials[mesh_index_material_index[i]]));
		}
		return meshes_materials;
	}

private:
	void loadModel_ext_format(const char* filedirIN, bool loadTangents, bool optimizeMesh, bool smoothNormals = false);
	void loadModel_int_format(const char* filedirIN, bool loadTangents, bool optimizeMesh, bool smoothNormals = false);
	void processNode(void* assimp_aiNode, const void* assimp_aiScene, bool hastangents);
	void processScene(std::string directory, const void* assimp_aiScene, bool hastangents);
	Mesh* processMesh(void* assimp_aiMesh,bool hastangents);
	Material* processMaterial(void* assimp_aiMaterial, std::string directory);
	//using void* because assimp not in header file, will get derefrenced in the function as aiNode*,aiScene*,aiMesh*,etc
	std::vector<Mesh*> meshes;
	std::vector<Material*> materials;
	std::unordered_map<unsigned int, unsigned int> mesh_index_material_index;
	//todo complete implementation after material is implemented
};

*/