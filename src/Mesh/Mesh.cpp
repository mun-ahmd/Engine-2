#include "Mesh.h"
#include <string>
#include <fstream>
#include <unordered_map>
#include <iostream>
#include "error.h"

using namespace std;





std::string getFileExtention(std::string filename)
{
	size_t pos1 = filename.find_last_of('.');
	if (pos1 == filename.npos)
		return "invalidFileName";
	return filename.substr(pos1 + 1, filename.size() - pos1 - 1);
}

void Mesh3::setupMesh(float* pos, float* norms, float* uv, float* additionalPerVertexData,unsigned int* face_indices, size_t len)
{
	//in mesh3 additional per vertexdata is ignored completely
}

void Mesh3::setupMesh(float* interleaved_data, unsigned int* face_indices, size_t len)
{
	//todo after graphics interface is done
	if (len*sizeof(float) % (sizeof(Vertex3)) != 0)
	{
		ENGINE2_THROW_ERROR("Mesh not setup due to interleaved_data being invalid");
		return;
	}

	this->indices.reserve(len / 3);
	for (unsigned int i = 0; i < len / 3; ++i)
		this->indices.push_back(*(face_indices + i));

	this->data.reserve(len/(sizeof(Vertex3)/sizeof(float)));
	for (size_t i = 0; i < len; ++i)
	{
		Vertex3 vert;
		glm::vec3 pos(interleaved_data[i], interleaved_data[i+1], interleaved_data[i+2]);
		glm::vec3 norm(interleaved_data[i+3], interleaved_data[i + 4], interleaved_data[i + 5]);
		glm::vec2 uv(interleaved_data[i + 6], interleaved_data[i + 7]);
		i += 7;
		vert.pos = pos;
		vert.norm = norm;
		vert.uv = uv;
		this->data.push_back(vert);
	}
	//Vertex3* dataINCasted = reinterpret_cast<Vertex3*>(interleaved_data);	//todo might be dangerous, test well
	//data = std::vector<Vertex3>(dataINCasted, dataINCasted+len);
	this->dataLoaded = true;
}

VertexData Mesh3::getMeshData()
{
	VertexData data;
	data.type = this->dataType;
	data.size = this->data.size();
	data.data = &(this->data[0].pos[0]);
	return data;
}