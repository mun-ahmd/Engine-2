#pragma once

#include "glm/glm.hpp"
#include <vector>


enum Vertex_Data_Type
{
	pos_norm_uv,
	pos_norm_tan_uv	//todo implement mesh for this type
};

struct VertexData
{
	//todo not sure about returning this pointer to the data, maybe should make it const and provide another means to changing it
	Vertex_Data_Type type;
	size_t size;
	float* data;
};

class Mesh
{
protected:
	std::vector<unsigned int> indices;
public:
	virtual void setupMesh(float* pos, float* norms, float* uv, float* additionalPerVertexData, unsigned int* face_indices, size_t len) = 0;
	virtual void setupMesh(float* interleaved_data, unsigned int* face_indices, size_t len) = 0;
	virtual VertexData getMeshData() = 0;
};


struct Vertex3
{
	glm::vec3 pos;
	glm::vec3 norm;
	glm::vec2 uv;
};

class Mesh3 : public Mesh
{
private:
	bool dataLoaded = false;
	Vertex_Data_Type dataType = pos_norm_uv;
	std::vector<Vertex3> data;
public:
	void setupMesh(float* pos, float* norms, float* uv, float* additionalPerVertexData,unsigned int* face_indices, size_t len);
	void setupMesh(float* data,unsigned int* face_indices, size_t len);	//only accepts triangles
	VertexData getMeshData();
};