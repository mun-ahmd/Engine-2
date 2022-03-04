#include "Mesh.h"
#include "Graphics/HigherGraphics_2.h"
#include "glm/gtc/matrix_transform.hpp"

constexpr unsigned short static_meshes_buffer_size_in_megabytes = 200;
static MultiStaticMesh static_meshes_container(static_meshes_buffer_size_in_megabytes * 1000000, static_meshes_buffer_size_in_megabytes * 1000000);


MultiStaticMesh& MeshStatic::get_static_meshes_holder()
{
	return static_meshes_container;
}
void MeshStatic::prepare_indirect_draw_buffer()
{
	static_meshes_container.create_indirect_draw_buffer();
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	//static_meshes_container.debug_output_indirect_draw("compute_debug.csv");
}

void MeshStatic::setup_mesh()
{
	auto optional_mesh_id = static_meshes_container.add_mesh(this->vertices.data(), this->vertices.size(), this->indices.data(), this->indices.size());
	if (optional_mesh_id.has_value() == false) {
		//todo eroor handle
		std::cerr << "Could not setup static mesh buffer";
		exit(220);
	}
	this->static_mesh_id = optional_mesh_id.value();
}

void MeshStatic::unload() {
	static_meshes_container.remove_mesh(this->static_mesh_id);
}

void MeshStatic::draw() {
	//this is only a draw call nothing else is done
	static_meshes_container.draw_alone(this->static_mesh_id);
}