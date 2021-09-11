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

void Mesh3::setupMesh(float* interleaved_data, size_t interleaved_data_size, unsigned int* face_indices, size_t num_indices)
{
	//todo after graphics interface is done
	//if (len*sizeof(float) % (sizeof(Vertex3)) != 0)
	//{
	//	ENGINE2_THROW_ERROR("Mesh not setup due to interleaved_data being invalid");
	//	return;
	//}
	this->indices = std::vector<unsigned int>(face_indices, face_indices + num_indices);

	verts.reserve(interleaved_data_size / (sizeof(Vertex3) / sizeof(float)));
	for (size_t i = 0; i < interleaved_data_size; i += sizeof(Vertex3) / sizeof(float))
	{
		Vertex3 curr_vert;
		verts.push_back(curr_vert);
		verts.back().pos = glm::vec3(interleaved_data[i], interleaved_data[i + 1], interleaved_data[i + 2]);
		verts.back().norm = glm::vec3(interleaved_data[i + 3], interleaved_data[i + 4], interleaved_data[i + 5]);
		verts.back().uv = glm::vec2(interleaved_data[i + 6], interleaved_data[i + 7]);
	}
	//Vertex3* dataINCasted = reinterpret_cast<Vertex3*>(interleaved_data);	//todo might be dangerous, test well
	//data = std::vector<Vertex3>(dataINCasted, dataINCasted+len);

	setupVAO();
	this->dataLoaded = true;
}
