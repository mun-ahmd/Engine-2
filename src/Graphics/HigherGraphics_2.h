#pragma once
#include "Graphics_2.h"
#include <vector>

#include "Mesh/Mesh.h"


class BufferMulti
{
private:
	std::unordered_map<unsigned int, unsigned int> id_to_index;	//in most cases id would be vertex array id
	std::vector<std::pair<size_t, unsigned int>> offsets_ids;
	size_t size_used;
	Buffer buff;
	size_t buffer_size;

public:

	BufferMulti() : buffer_size(0)
	{

	}
	BufferMulti(size_t buffer_size) : buffer_size(buffer_size), size_used(0)
	{
		buff = Buffer(buffer_size, NULL);
	}

	void change_id(unsigned int old_id, const VertexArray& new_vao)
	{
		size_t index_ = id_to_index[old_id];
		id_to_index.erase(old_id);
		id_to_index[new_vao.get_id()] = index_;
		offsets_ids[index_].second = new_vao.get_id();
	}

	inline unsigned int num_objects() { return offsets_ids.size(); }
	inline size_t get_used_size() const{ return size_used; }
	std::vector<size_t> get_all_offsets() const
	{
		std::vector<size_t> offsets_all;
		offsets_all.reserve(offsets_ids.size());
		for (auto ptr = offsets_ids.begin(); ptr < offsets_ids.end(); ++ptr)
		{
			offsets_all.push_back(ptr->first);
		}
		return offsets_all;
	}

	bool allocate_next(unsigned int id, const void* data, size_t data_size)
	{
		if (id_to_index.count(id) > 0)
		{
			return false;
		}
		if ((buffer_size - size_used) < data_size)
		{
			return false;
		}
		buff.modify(data, data_size, size_used);
		offsets_ids.push_back(std::pair(size_used, id));
		size_used += data_size;
		id_to_index[id] = offsets_ids.size() - 1;
		return true;
	}

	bool deallocate(unsigned int id)
	{
		size_t deallocate_offset_index = id_to_index[id];
		if (deallocate_offset_index == offsets_ids.size() - 1)
		{
			offsets_ids.pop_back();
			id_to_index.erase(id);
			return true;
		}
		buff.copy_self(size_used - offsets_ids[deallocate_offset_index + 1].first, offsets_ids[deallocate_offset_index].first, offsets_ids[deallocate_offset_index + 1].first);
		//todo ^ might need to change it to make a new intermediate buffer
		size_t removed_size = offsets_ids[deallocate_offset_index + 1].first - offsets_ids[deallocate_offset_index].first;
		size_used -= removed_size;
		offsets_ids.erase(std::next(offsets_ids.begin(), deallocate_offset_index));
		id_to_index.erase(id);

		for (auto ptr = std::next(offsets_ids.begin(), deallocate_offset_index); ptr < offsets_ids.end(); ++ptr)
		{
			ptr->first -= removed_size;
			id_to_index[ptr->second] -= 1;
		}
		return true;
	}

	bool deallocate(const VertexArray& vao)
	{
		unsigned int id = vao.get_id();
		size_t deallocate_offset_index = id_to_index[id];
		if (deallocate_offset_index == offsets_ids.size() - 1)
		{
			offsets_ids.pop_back();
			id_to_index.erase(id);
			return true;
		}
		buff.copy_self(size_used - offsets_ids[deallocate_offset_index + 1].first, offsets_ids[deallocate_offset_index].first, offsets_ids[deallocate_offset_index + 1].first);
		//todo ^ might need to change it to make a new intermediate buffer
		size_t removed_size = offsets_ids[deallocate_offset_index + 1].first - offsets_ids[deallocate_offset_index].first;
		size_used -= removed_size;
		offsets_ids.erase(std::next(offsets_ids.begin(), deallocate_offset_index));
		id_to_index.erase(id);

		for (auto ptr = std::next(offsets_ids.begin(), deallocate_offset_index); ptr < offsets_ids.end(); ++ptr)
		{
			ptr->first -= removed_size;
			id_to_index[ptr->second] -= 1;
		}
		return true;
	}

	inline size_t get_object_offset(unsigned int id) const
	{
		return offsets_ids[id_to_index.at(id)].first;
	}

	inline size_t get_object_offset(const VertexArray& vao) const
	{
		return offsets_ids[id_to_index.at(vao.get_id())].first;
	}

	inline size_t get_object_index(const VertexArray& vao) const
	{
		return id_to_index.at(vao.get_id());
	}

	inline Buffer get_buffer() { return buff; }




};


class MultiStaticMesh
{
private:
	BufferMulti indices_multi;
	BufferMulti vertices_multi;
	unsigned int num_meshes = 0;
	VertexArray VAO;
	Buffer indirect_draw_buffer;
	Buffer transform_matrices_buffer;
	Buffer material_ids_buffer;
	Buffer mesh_instance_info_buffer;
	unsigned int base_instance_vertex_attribute_index = 3;
	unsigned int num_clusters = 0;

	const unsigned int num_triangles_per_cluster = 128;
	const unsigned char indices_type_size = 4;
	const unsigned short per_vertex_size = sizeof(float) * 8;

	ComputeShader culling_compute = ComputeShader("CullClustersCompute.glsl");

	std::vector<VertexAttribData> get_vertex_attribs()
	{
		std::vector<VertexAttribData> attribs(3);
		attribs[0].type = GL_FLOAT;
		attribs[0].normalized = false;
		attribs[0].attrib_size = 3;
		attribs[0].offset = sizeof(unsigned int)*0;
		attribs[0].stride = 8 * sizeof(float);

		attribs[1] = attribs[0];
		attribs[1].normalized = true;
		attribs[1].offset += 3 * sizeof(float);

		attribs[2] = attribs[0];
		attribs[2].attrib_size = 2;
		attribs[2].offset += 6 * sizeof(float);

		//attribs[3].type = GL_UNSIGNED_INT;
		//attribs[3].attrib_size = 1;
		//attribs[3].offset = 0;
		//attribs[3].stride = sizeof(unsigned int);	//todo this wont work; google if you dont understand why
		//attribs[3].normalized = false;

		//attribs[3] = attribs[0];
		//attribs[3].type = GL_UNSIGNED_INT;
		//attribs[3].attrib_size = 1;
		//attribs[3].offset += 8 * sizeof(float);

		return attribs;
	}

	void set_transform(unsigned int index)
	{
		glm::mat4 zero_transform = glm::mat4(1.0f);
		transform_matrices_buffer.modify(&zero_transform, sizeof(glm::mat4), index * sizeof(glm::mat4));
	}

public:
	MultiStaticMesh(unsigned int num_triangles_per_cluster, unsigned char indices_type_size, unsigned short per_vertex_size) :
		num_triangles_per_cluster(num_triangles_per_cluster), indices_type_size(indices_type_size), per_vertex_size(per_vertex_size)
	{}
	void initialize(size_t max_size_vertex_buff, size_t max_size_index_buff)
	{
		vertices_multi = BufferMulti(max_size_vertex_buff);
		indices_multi = BufferMulti(max_size_index_buff);
		VAO = VertexArray(vertices_multi.get_buffer(), indices_multi.get_buffer(), get_vertex_attribs());
		//glVertexAttribDivisor(3, 1);
		indirect_draw_buffer = Buffer(0, NULL, GL_ARRAY_BUFFER, true);
		transform_matrices_buffer = Buffer(sizeof(glm::mat4) * 1000, NULL, GL_SHADER_STORAGE_BUFFER, 0, GL_STREAM_DRAW);
		material_ids_buffer = Buffer(sizeof(unsigned int) * 1000, NULL, GL_SHADER_STORAGE_BUFFER, 0, GL_STREAM_DRAW);
	}
	void multi_draw() const
	{
		VAO.multi_draw_indirect(num_clusters, 0, 0);
	}
	const Buffer& get_indirect_draw_buffer() const
	{
		return indirect_draw_buffer;
	}

	void compute_indirect_draw_buffer()
	{

	}

	void create_indirect_draw_buffer()
	{
		//prepares data cpu side , no compute shader, use this for debugging
		auto verts_offsets = vertices_multi.get_all_offsets();
		auto indices_offsets = indices_multi.get_all_offsets();
		size_t num_indices = indices_multi.get_used_size() / indices_type_size;


		struct indirect_draw_struct
		{
			unsigned int  count;
			unsigned int  instanceCount;
			unsigned int  firstIndex;
			unsigned int  baseVertex;
			unsigned int  baseInstance;
		};

		std::vector<indirect_draw_struct> buff_data;
		buff_data.reserve(num_indices / (3 * num_triangles_per_cluster));	//want x triangle groups, that being 3*x indices per group

		//for last mesh to also get
		indices_offsets.push_back(indices_multi.get_used_size());
		for (size_t i = 0; i < indices_offsets.size() - 1; ++i)
		{
			unsigned int baseVertex = (verts_offsets[i]) / per_vertex_size;

			if ((indices_offsets[i + 1] - indices_offsets[i]) / indices_type_size > num_triangles_per_cluster * 3)
			{
				for (unsigned int x = 0; x < (indices_offsets[i + 1] - indices_offsets[i]) / indices_type_size; x += num_triangles_per_cluster * 3)
				{
					indirect_draw_struct curr;
					curr.count = num_triangles_per_cluster * 3;
					curr.instanceCount = 1;
					curr.baseVertex = baseVertex;
					curr.firstIndex = indices_offsets[i] / indices_type_size + x;
					curr.baseInstance = i;
					buff_data.push_back(curr);
				}
			}
			else
			{
				indirect_draw_struct curr;
				curr.count = (indices_offsets[i + 1] - indices_offsets[i]) / indices_type_size;
				curr.instanceCount = 1;
				curr.baseVertex = baseVertex;
				curr.firstIndex = indices_offsets[i] / indices_type_size;
				curr.baseInstance = i;
				buff_data.push_back(curr);
			}
		}
		
		indirect_draw_buffer.new_data(buff_data.data(), buff_data.size() * sizeof(indirect_draw_struct));
	}
	std::pair<VertexArray, size_t> add_mesh(
		const void* vertices_data, size_t vertices_data_size, const void* indices_data, size_t indices_data_size, std::vector<VertexAttribData> attribs
	);
	void set_transform(unsigned int index, glm::mat4 transform)
	{
		transform_matrices_buffer.modify(&transform, sizeof(glm::mat4), index * sizeof(glm::mat4));
	}
	void set_transform(MeshStatic* mesh, glm::mat4 transform)
	{
		transform_matrices_buffer.modify(&transform, sizeof(glm::mat4), vertices_multi.get_object_index(mesh->get_vao()) * sizeof(glm::mat4));
	}
	void set_mat_id(MeshStatic* mesh, unsigned int  mat_id)
	{
		material_ids_buffer.modify(&mat_id, sizeof(unsigned int), vertices_multi.get_object_index(mesh->get_vao()) * sizeof(unsigned int));
	}


	Buffer get_transform_buffer() const
	{
		return transform_matrices_buffer;
	}
};

class HigherGraphics
{
public:
	static void initialize();
	static void prepare_indirect_draw_buffer();
	static const MultiStaticMesh& get_static_meshes_holder();
	static void add_instance_of_mesh(MeshStatic* mesh, glm::vec3 position);
};