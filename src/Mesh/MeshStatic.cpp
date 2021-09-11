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
	auto vao_ioffset = static_meshes_container.add_mesh(vertices.data(), vertices.size() * sizeof(Vertex3), indices.data(), indices.size() * sizeof(unsigned int), attribs);
	VAO = vao_ioffset.first;
	indices_offset = vao_ioffset.second;
}