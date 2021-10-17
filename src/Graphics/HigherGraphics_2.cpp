#include "HigherGraphics_2.h"

static unsigned int placeholder_id_vertices = std::numeric_limits<unsigned int>::max() / 2;
static unsigned int placeholder_id_indices = std::numeric_limits<unsigned int>::max() / 2;

namespace ComputeShaderDataStructures
{
	struct ClusterInfo
	{
		unsigned int mesh_id;
		unsigned int num_triangles;
	};

	struct MeshInformation
	{
		unsigned int num_instances;
		unsigned int base_vertex;
	};
}


std::pair<VertexArray, long long> MultiStaticMesh::add_mesh(
	const void* vertices_data, size_t vertices_data_size,
	const void* indices_data,
	size_t indices_data_size,
	unsigned int num_triangles_actual,
	std::vector<VertexAttribData> attribs)
{
	unsigned int num_triangles_in_mesh = indices_data_size /(3*this->indices_type_size);
	

	if (num_triangles_in_mesh % this->num_triangles_per_cluster != 0)
	{
		//reject mesh
		return std::pair(VertexArray(), -1);
	}
	while (vertices_multi.allocate_next(placeholder_id_vertices, vertices_data, vertices_data_size ) == false)
	{
		//todo Hello thread safety :)
		placeholder_id_vertices += 1;
	}
	while (indices_multi.allocate_next(placeholder_id_indices, indices_data, indices_data_size) == false)
	{
		placeholder_id_indices += 1;	
	}

	size_t indices_offset = indices_multi.get_object_offset(placeholder_id_indices);

	for (auto ptr = attribs.begin();ptr < attribs.end();++ptr)
	{
		ptr->offset += vertices_multi.get_object_offset(placeholder_id_vertices);
	}

	VertexArray Mesh_VAO(vertices_multi.get_buffer(),
		indices_multi.get_buffer(),
		attribs
	);

	vertices_multi.change_id(placeholder_id_vertices, Mesh_VAO);
	indices_multi.change_id(placeholder_id_indices, Mesh_VAO);

	size_t num_indices_curr = indices_data_size / indices_type_size;
	unsigned int mesh_num_clusters = num_indices_curr / (3 * num_triangles_per_cluster);
	this->num_clusters += mesh_num_clusters;
	if (num_indices_curr % num_triangles_per_cluster != 0) num_clusters += 1;


	// initialize per cluster and per mesh information
	std::vector<ComputeShaderDataStructures::ClusterInfo> clusters_info(mesh_num_clusters);
	for (auto itr = clusters_info.begin(); itr < clusters_info.end()-1; ++itr)
	{
		itr->mesh_id = num_meshes;
		itr->num_triangles = 128;
	}
	clusters_info.back().mesh_id = num_meshes;
	clusters_info.back().num_triangles = 128 - (num_triangles_in_mesh - num_triangles_actual);
	if (this->cluster_information_buffer.allocate_next(
		Mesh_VAO.get_id(),
		clusters_info.data(),
		clusters_info.size() * sizeof(ComputeShaderDataStructures::ClusterInfo)
	) == false
		){
		//error
		std::cerr << "Could not allocate cluster information buffer for static mesh: " << Mesh_VAO.get_id();
		exit(12);
	}

	ComputeShaderDataStructures::MeshInformation mesh_info;
	mesh_info.base_vertex = this->vertices_multi.get_object_offset(Mesh_VAO)/this->per_vertex_size;
	mesh_info.num_instances = 1;

	if (this->mesh_information_buffer.allocate_next(Mesh_VAO.get_id(),
		&mesh_info,
		sizeof(mesh_info)) == false
		) {
		//error
		std::cerr << "Could not allocate mesh information buffer for static mesh: " << Mesh_VAO.get_id();
		exit(12);
	}


	set_transform(num_meshes);
	num_meshes += 1;

	return std::pair(Mesh_VAO, indices_offset);
}
