#pragma once

#include "glm/glm.hpp"
#include "Graphics/Graphics_2.h"
#include <vector>
#include <any>
#include <optional>

enum Vertex_Data_Type
{
	pos_norm_uv,
	pos_norm_tan_uv	//todo implement mesh for this type
};

struct VertexData
{
	//todo not sure about returning this pointer to the data, maybe should make it const and provide another means to changing it
	Vertex_Data_Type type;
	size_t verts_data_size;
	float* verts_data;
	size_t indices_data_size;
	unsigned int* indices_data;
};


struct VertexDataAny {
	std::optional<std::vector<glm::vec3>> positions;
	std::optional<std::vector<glm::vec3>> normals;
	std::optional<std::vector<glm::vec2>> uvs;
	std::optional<std::any> others;
	VertexDataAny() = default;
};

class Mesh
{
protected:
public:
	virtual ~Mesh() {}
	virtual Vertex_Data_Type get_vertex_data_type() = 0;
	virtual VertexDataAny get_vertex_data() = 0;
	virtual std::vector<unsigned int> get_indices() = 0;
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
	std::vector<Vertex3> verts;
	std::vector<unsigned int> indices;
	VertexArray VAO;
	void setupVAO()
	{
		std::vector<VertexAttribData> attribs(3);
		attribs[0].type = GL_FLOAT;
		attribs[0].normalized = false;
		attribs[0].attrib_size = 3;
		attribs[0].offset = 0;
		attribs[0].stride = 8 * sizeof(float);

		attribs[1] = attribs[0];
		attribs[1].normalized = true;
		attribs[1].offset = 3 * sizeof(float);

		attribs[2] = attribs[0];
		attribs[2].attrib_size = 2;
		attribs[2].offset = 6 * sizeof(float);

		VAO = VertexArray(sizeof(Vertex3) * this->verts.size(), this->verts.data(), indices.size() * sizeof(unsigned int), indices.data(), attribs, GL_STATIC_DRAW);
	}
public:
	Vertex_Data_Type get_vertex_data_type() override {
		return pos_norm_uv;
	}
	VertexDataAny get_vertex_data() override {
		VertexDataAny v_data;
		v_data.positions = std::vector<glm::vec3>();
		v_data.positions->reserve(this->verts.size());

		v_data.normals = std::vector<glm::vec3>();
		v_data.normals->reserve(this->verts.size());

		v_data.uvs = std::vector<glm::vec2>();
		v_data.uvs->reserve(this->verts.size());

		for (auto p = this->verts.begin(); p < this->verts.end(); ++p)
		{
			v_data.positions->push_back(p->pos);
			v_data.normals->push_back(p->norm);
			v_data.uvs->push_back(p->uv);
		}
		return v_data;
	}
	std::vector<unsigned int> get_indices() override {
		return this->indices;
	}
	void setupMesh(float* pos, float* norms, float* uv, float* additionalPerVertexData, unsigned int* face_indices, size_t len);
	void setupMesh(float* interleaved_data, size_t num_vertices, unsigned int* face_indices, size_t num_indices);	//only accepts triangles
	VertexData getMeshData()
	{
		VertexData data;
		data.type = pos_norm_uv;
		data.verts_data_size = verts.size() * (sizeof(Vertex3) / sizeof(float) );
		data.verts_data = (float*)verts.data();

		data.indices_data_size = indices.size();
		data.indices_data = indices.data();

		return data;
	}

	void draw()
	{
		VAO.draw(indices.size());
	}

	inline VertexArray get_vertex_array()
	{
		return this->VAO;
	}

	std::pair<std::vector<Vertex3>, std::vector<unsigned int>> debug_get_arrays()
	{
		return std::pair(verts, indices);
	}

};


//forward declaration
class MultiStaticMesh;

class MeshStatic
{
private:
	std::vector<Vertex3> vertices;
	std::vector<unsigned int> indices;

	uint32_t static_mesh_id;

	void setup_mesh();

	static std::vector<VertexAttribData> get_vertex_attribs(size_t base_offset)
	{
		std::vector<VertexAttribData> attribs(3);
		attribs[0].type = GL_FLOAT;
		attribs[0].normalized = false;
		attribs[0].attrib_size = 3;
		attribs[0].offset = base_offset;
		attribs[0].stride = 8 * sizeof(float);

		attribs[1] = attribs[0];
		attribs[1].normalized = true;
		attribs[1].offset += offsetof(Vertex3, norm);

		attribs[2] = attribs[0];
		attribs[2].attrib_size = 2;
		attribs[2].offset += offsetof(Vertex3, uv);

		return attribs;
	}
public:
	static void prepare_indirect_draw_buffer();
	static MultiStaticMesh& get_static_meshes_holder();


	MeshStatic(std::vector<Vertex3> vertices, std::vector<unsigned int> indices) : vertices(vertices), indices(indices)
	{
		setup_mesh();
		std::cout << "Mesh Size : " << sizeof(Vertex3) * vertices.size() + sizeof(unsigned int) * indices.size() << std::endl;
	}
	static Buffer debug_get_multi_draw_buff();
	static void multi_draw_static_meshes(Buffer indirect_buffer, unsigned int indirect_buffer_offset = 0);
	void draw();
	void unload();

	inline uint32_t get_static_mesh_id() { return this->static_mesh_id; }
};