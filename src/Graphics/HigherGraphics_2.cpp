#include "HigherGraphics_2.h"

static unsigned int placeholder_id_vertices = std::numeric_limits<unsigned int>::max() / 2;
static unsigned int placeholder_id_indices = std::numeric_limits<unsigned int>::max() / 2;



std::pair<VertexArray, size_t> MultiStaticMesh::add_mesh(const void* vertices_data, size_t vertices_data_size, const void* indices_data, size_t indices_data_size, std::vector<VertexAttribData> attribs)
{

	unsigned char* final_vertices_data = new unsigned char[vertices_data_size + sizeof(unsigned int)];
	memcpy(final_vertices_data,&num_meshes,sizeof(unsigned int));
	memcpy(final_vertices_data+sizeof(unsigned int), vertices_data, vertices_data_size);

	while (vertices_multi.allocate_next(placeholder_id_vertices, vertices_data, vertices_data_size ) == false)
	{
		placeholder_id_vertices += 1;
	}
	delete[](final_vertices_data);
	while (indices_multi.allocate_next(placeholder_id_indices, indices_data, indices_data_size) == false)
	{
		placeholder_id_indices += 1;
	}

	size_t indices_offset = indices_multi.get_object_offset(placeholder_id_indices);

	for (auto ptr = attribs.begin();ptr < attribs.end();++ptr)
	{
		ptr->offset += vertices_multi.get_object_offset(placeholder_id_vertices);
	}

	VertexArray VAO(vertices_multi.get_buffer(),
		indices_multi.get_buffer(),
		attribs
	);

	vertices_multi.change_id(placeholder_id_vertices, VAO);
	indices_multi.change_id(placeholder_id_indices, VAO);

	size_t num_indices_curr = indices_data_size / indices_type_size;
	num_clusters += num_indices_curr / (3*num_triangles_per_cluster);
	if (num_indices_curr % num_triangles_per_cluster != 0) num_clusters += 1;

	set_transform(num_meshes);
	num_meshes += 1;

	return std::pair(VAO, indices_offset);
}

