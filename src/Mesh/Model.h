#pragma once

#include "Mesh.h"
#include <vector>

//model is intended to be an imported asset, from model you can choose meshes as mesh components and materials for the renderer
class Model
{
public:
	void loadModel(const char* filedirIN,bool loadTangents, bool optimizeMesh,bool smoothNormals = false);
	
private:
	void processNode(void* assimp_aiNode, const void* assimp_aiScene, bool hastangents);
	void processScene(const void* assimp_aiScene, bool hastangents);
	Mesh* processMesh(void* assimp_aiMesh,bool hastangents);
	//using void* because assimp not in header file, will get derefrenced in the function as aiNode*,aiScene*,aiMesh*,etc
	std::vector<Mesh*> meshes;
	//std::vector<Material*> materials
	//todo complete implementation after material is implemented
};