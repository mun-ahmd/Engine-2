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

template<typename T>
struct MeshData {
	std::vector<T> vertices;
	std::vector<unsigned int> indices;
	std::optional<std::any> other;
	MeshData() = default;
	MeshData(std::vector<T> vertices, std::vector<unsigned int> indices) : vertices(vertices), indices(indices) {};
};


class Mesh
{
protected:
public:
	virtual ~Mesh() {}
	virtual std::any get_mesh_data_pointer() = 0;
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
	MeshData<Vertex3> data;
	Buffer vertex_buff;
	Buffer index_buff;
	VertexArray VAO;

	void setup_VAO()
	{
		std::vector<VertexAttribData> attribs = { 
			VertexAttribData(3, GL_FLOAT, false, 8 * sizeof(float), 0),
			VertexAttribData(3, GL_FLOAT, true, 8 * sizeof(float), 3 * sizeof(float)),
			VertexAttribData(2, GL_FLOAT, false, 6 * sizeof(float), 0) 
		};

		//std::vector<VertexAttribData> attribs(3);
		//attribs[0].type = GL_FLOAT;
		//attribs[0].normalized = false;
		//attribs[0].attrib_size = 3;
		//attribs[0].offset = 0;
		//attribs[0].stride = 8 * sizeof(float);

		//attribs[1] = attribs[0];
		//attribs[1].normalized = true;
		//attribs[1].offset = 3 * sizeof(float);

		//attribs[2] = attribs[0];
		//attribs[2].attrib_size = 2;
		//attribs[2].offset = 6 * sizeof(float);

		VAO = VertexArray(vertex_buff, index_buff, attribs);
	}

public:
	typedef const MeshData<Vertex3>* MeshDataPointerType;
	~Mesh3() {

	}

	Mesh3(const MeshData<Vertex3>&& data) : data(data),
		index_buff(Buffer(sizeof(unsigned int)* data.indices.size(), data.indices.data())),
		vertex_buff(Buffer(sizeof(Vertex3)* data.vertices.size(), data.vertices.data())) {
		VAO = VertexArray(vertex_buff, index_buff, {
			VertexAttribData(3, GL_FLOAT, false, 8 * sizeof(float), 0),
			VertexAttribData(3, GL_FLOAT, true, 8 * sizeof(float), 3 * sizeof(float)),
			VertexAttribData(2, GL_FLOAT, false, 6 * sizeof(float), 0)
			});
	}
	Mesh3(const MeshData<Vertex3>& data) : data(data),
		index_buff(Buffer(sizeof(unsigned int)* data.indices.size(), data.indices.data())),
		vertex_buff(Buffer(sizeof(Vertex3)* data.vertices.size(), data.vertices.data())) {
		VAO = VertexArray(vertex_buff, index_buff, {
			VertexAttribData(3, GL_FLOAT, false, 8 * sizeof(float), 0),
			VertexAttribData(3, GL_FLOAT, true, 8 * sizeof(float), 3 * sizeof(float)),
			VertexAttribData(2, GL_FLOAT, false, 6 * sizeof(float), 0)
			});
	}

	std::any get_mesh_data_pointer() override {
		return std::make_any<MeshDataPointerType>(static_cast<MeshDataPointerType>(&this->data));
	}
	inline const MeshData<Vertex3>& get_mesh_data(){
		return this->data;
	}
	inline void draw(){
		VAO.draw(data.indices.size());
	}
	inline VertexArray get_vertex_array(){
		return this->VAO;
	}
	inline void destroy() {
		this->VAO.destroy();
	}

};

//forward declaration
class MultiStaticMesh;

class MeshStatic
{
private:
	MeshData<Vertex3> mesh;

	uint32_t static_mesh_id;

	void setup_mesh();


public:
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

	static void prepare_indirect_draw_buffer();
	static MultiStaticMesh& get_static_meshes_holder();

	MeshStatic(const MeshData<Vertex3>&& data) : mesh(data)
	{
		setup_mesh();
	}
	MeshStatic(const MeshData<Vertex3>& data) : mesh(data)
	{
		setup_mesh();
	}

	static Buffer debug_get_multi_draw_buff();
	static void multi_draw_static_meshes(Buffer indirect_buffer, unsigned int indirect_buffer_offset = 0);
	void draw();
	void unload();

	inline uint32_t get_static_mesh_id() { return this->static_mesh_id; }
};

struct MaterialPBR {
  std::string metallicRoughnessTex = "";
  float metallic_factor = 1.0;
  float roughness_factor = 1.0;
  std::string baseColorTex = "";
  glm::vec4 baseColorFactor = glm::vec4(1.0);
  std::string normalMap = "";
  float normalScale = 1.0;
};

struct ModelData{
  std::vector<std::pair<MeshData<Vertex3>, int>> meshes;
  std::vector<MaterialPBR> materials;
};


std::optional<ModelData> loadGLTF(const char *filepath);
