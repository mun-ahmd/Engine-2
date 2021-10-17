#include "Mesh.h"
#include "Graphics/HigherGraphics_2.h"
#include "glm/gtc/matrix_transform.hpp"

constexpr unsigned short static_meshes_buffer_size_in_megabytes = 200;
static MultiStaticMesh static_meshes_container(128, sizeof(unsigned int), sizeof(Vertex3));

void HigherGraphics::initialize()
{
	static_meshes_container.initialize(static_meshes_buffer_size_in_megabytes * 1000000, static_meshes_buffer_size_in_megabytes * 1000000);
}
const MultiStaticMesh& HigherGraphics::get_static_meshes_holder()
{
	return static_meshes_container;
}
void HigherGraphics::prepare_indirect_draw_buffer()
{
	static_meshes_container.compute_indirect_draw_buffer();
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	//static_meshes_container.debug_output_indirect_draw("compute_debug.csv");
}

void HigherGraphics::add_instance_of_mesh(MeshStatic* mesh, glm::vec3 position)
{
	static_meshes_container.set_transform(mesh, glm::translate(glm::mat4(1), position));
}

void MeshStatic::setup_mesh()
{
	std::vector<VertexAttribData> attribs(3);
	attribs[0].type = GL_FLOAT;
	attribs[0].normalized = false;
	attribs[0].attrib_size = 3;
	attribs[0].offset = 0;
	attribs[0].stride = 8 * sizeof(float);

	attribs[1] = attribs[0];
	attribs[1].normalized = true;
	attribs[1].offset += 3 * sizeof(float);

	attribs[2] = attribs[0];
	attribs[2].attrib_size = 2;
	attribs[2].offset += 6 * sizeof(float);

	//make sure vertex data can be used for clusters, that is it has appropriate number of triangles
	this->num_triangles_actual = (indices.size() / 3);
	unsigned short padding_required = static_meshes_container.get_num_triangles_per_cluster() - 
		((this->num_triangles_actual + static_meshes_container.get_num_triangles_per_cluster()) %
			static_meshes_container.get_num_triangles_per_cluster());
	if (padding_required != 0)
	{
		Vertex3 NaN_Vertex;
		float NaN = nanf("");
		NaN_Vertex.pos = glm::vec3(NaN,NaN,NaN);
		NaN_Vertex.norm = NaN_Vertex.pos;
		NaN_Vertex.uv = glm::vec2(NaN, NaN);
		vertices.push_back(NaN_Vertex);
		for (unsigned short i = 0; i < padding_required*3; ++i)
			indices.push_back(vertices.size()-1);
	}

	auto vao_ioffset = static_meshes_container.add_mesh(
		vertices.data(),
		vertices.size() * sizeof(Vertex3),
		indices.data(),
		indices.size() * sizeof(unsigned int),
		this->num_triangles_actual,
		attribs);
	if (vao_ioffset.second < 0)
	{
		//error
		std::cerr << "Static Mesh Indices Data Not Padded Or Incorrectly Padded, unable to get clusters with equal number of triangles";
		exit(12);
	}
	VAO = vao_ioffset.first;
	indices_offset = vao_ioffset.second;
}