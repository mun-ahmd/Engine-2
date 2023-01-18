#pragma once
#include "Graphics_2.h"
#include <type_traits>
#include <vector>
#include <list>
#include "entt/entt.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "Mesh/Mesh.h"
#include <limits>
#include "entt/entt.hpp"
#include <string>
#include <unordered_map>
#include "Graphics/cameraObj.h"


static_assert(std::is_same<uint32_t, entt::id_type>::value);
typedef entt::entity MaterialEntity;

class RenderPass {
public:
  //init should be called before the first frame, once throughout execution
  virtual void init(entt::registry &registry) = 0;
  //execute should perform per-frame actions
  virtual void execute(entt::registry &registry) = 0;
};

typedef glm::vec3 Position;

class CamHandler
{
private:
	GLFWwindow* currentWindow;
	double mouseLastX;
	double mouseLastY;
	bool hasMouseMovedOnce;
	Camera cam;
public:
	CamHandler(GLFWwindow* window)
	{
		this->cam = Camera(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0, 1, 0));
		currentWindow = window;
		hasMouseMovedOnce = 0;
		mouseLastX = 0.0;
		mouseLastY = 0.0;
		cam.movementSpeed /= 2;
	}
	void moveAround(double deltaTime)
	{
		if (glfwGetKey(this->currentWindow, GLFW_KEY_W) == GLFW_PRESS)
			this->cam.keyboardMovementProcess(FORWARD, deltaTime);
		if (glfwGetKey(this->currentWindow, GLFW_KEY_S) == GLFW_PRESS)
			this->cam.keyboardMovementProcess(BACKWARD, deltaTime);
		if (glfwGetKey(this->currentWindow, GLFW_KEY_A) == GLFW_PRESS)
			this->cam.keyboardMovementProcess(LEFT, deltaTime);
		if (glfwGetKey(this->currentWindow, GLFW_KEY_D) == GLFW_PRESS)
			this->cam.keyboardMovementProcess(RIGHT, deltaTime);
	}
	void lookAround()
	{
		double xPos, yPos;
		double lastX = this->mouseLastX; double lastY = this->mouseLastY;
		glfwGetCursorPos(this->currentWindow, &xPos, &yPos);

		//if first mouse input....
		//this stops massive offset when starting due to large diff b/w last pos and curr pos of mouse
		if (this->hasMouseMovedOnce == false) {
			lastX = xPos;
			lastY = yPos;
			this->hasMouseMovedOnce = true;
		}

		double xOffset = xPos - lastX;
		double yOffset = yPos - lastY;

		lastX = xPos;
		lastY = yPos;
		this->mouseLastX = lastX; this->mouseLastY = lastY;

		this->cam.mouseLookProcess(xOffset, yOffset, true);
	}
	glm::mat4 getView()
	{
		return this->cam.getViewMatrix();
	}
	glm::vec3 get_pos() {
		return this->cam.position;
	}
};

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

	struct IndirectDrawInfo {
		unsigned int  count;
		unsigned int  instanceCount;
		unsigned int  firstIndex;
		unsigned int  baseVertex;
		unsigned int  baseInstance;
	};
}

constexpr Vertex3 NaN_Vertex = { glm::vec3(std::numeric_limits<float>::quiet_NaN()), glm::vec3(std::numeric_limits<float>::quiet_NaN()), glm::vec2(std::numeric_limits<float>::quiet_NaN()) };

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
		offsets_ids.push_back(std::make_pair(size_used, id));
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

class VertexArrayHasherClass {
	size_t operator()(const VertexArray& vao) {
		return std::hash<unsigned int>()(vao.get_id());
	}
};
class MultiStaticMesh {
private:
	Buffer indices;
	Buffer vertices;
	Buffer cluster_information;
	Buffer mesh_information;
	Buffer transformations;
	Buffer material_ids;

	VertexArray VAO;

	Buffer indirect_draw_info;

	struct MeshStoreOffsetInfo {
		size_t indices_offset;
		size_t vertices_offset;
		size_t clusters_offset;
		
		size_t indices_size;
		size_t vertices_size;

		int mesh_index;	//-1 can be invalid mesh_index aka deleted mesh
	};
	std::vector<MeshStoreOffsetInfo> mesh_buffers_info;
	//mesh_index is the same as static mesh id, because it is assured that it is unique despite deletion of meshes
	std::unordered_map <uint32_t, unsigned int> id_to_buffer_info;
	std::vector<unsigned int> deleted_meshes_buffer_info;

	unsigned int num_meshes = 0;
	unsigned int num_clusters = 0;

	ComputeShader culling_compute = ComputeShader("src/Graphics/Shaders/CullClusters.compute");

	inline size_t add_mesh_store_info(size_t vertices_data_size, size_t indices_data_size) {
		MeshStoreOffsetInfo info;
		if (mesh_buffers_info.empty()) {
			info.indices_offset = 0;
			info.vertices_offset = 0;
			info.clusters_offset = 0;
		}
		else {
			info.indices_offset = mesh_buffers_info.back().indices_offset + mesh_buffers_info.back().indices_size;
			info.vertices_offset = mesh_buffers_info.back().vertices_offset + mesh_buffers_info.back().vertices_size;
			size_t num_clusters_prev = mesh_buffers_info.back().indices_size / (3 * num_triangles_per_cluster);
			info.clusters_offset = mesh_buffers_info.back().clusters_offset + num_clusters_prev * sizeof(ComputeShaderDataStructures::ClusterInfo);
		}
		info.indices_size = indices_data_size;
		info.vertices_size = vertices_data_size;
		info.mesh_index = this->mesh_buffers_info.size();

		this->mesh_buffers_info.push_back(info);

		ComputeShaderDataStructures::MeshInformation mesh_inf;
		mesh_inf.num_instances = 0;
		mesh_inf.base_vertex = info.vertices_offset / sizeof(Vertex3);
		this->mesh_information.modify(
			&mesh_inf,
			sizeof(ComputeShaderDataStructures::MeshInformation),
			info.mesh_index * sizeof(ComputeShaderDataStructures::MeshInformation));

		// return the index to the information in the mesh buffers info array
		return mesh_buffers_info.size() - 1;
	}

	std::vector<VertexAttribData> get_vertex_attribs() const
	{
		std::vector<VertexAttribData> attribs(3);
		attribs[0].type = GL_FLOAT;
		attribs[0].normalized = false;
		attribs[0].attrib_size = 3;
		attribs[0].offset = sizeof(unsigned int) * 0;
		attribs[0].stride = 8 * sizeof(float);

		attribs[1] = attribs[0];
		attribs[1].normalized = true;
		attribs[1].offset += 3 * sizeof(float);

		attribs[2] = attribs[0];
		attribs[2].attrib_size = 2;
		attribs[2].offset += 6 * sizeof(float);

		return attribs;
	}

	
	
public:
	struct MeshDrawInfo {
		VertexArray vao;
		size_t first_index;
		size_t num_indices;
	};
	static constexpr unsigned int num_triangles_per_cluster = 128;
	static constexpr unsigned int max_num_meshes = 100;
	MultiStaticMesh(const size_t max_index_buff_size, const size_t max_vertex_buff_size) {

		this->indices = Buffer(max_index_buff_size, NULL);
		this->vertices = Buffer(max_vertex_buff_size, NULL);

		this->VAO = VertexArray(
			this->vertices,
			this->indices,
			this->get_vertex_attribs());

		size_t max_num_triangles = max_index_buff_size / (sizeof(unsigned int) * 3);
		size_t max_num_clusters = max_num_triangles / num_triangles_per_cluster;
		
		this->mesh_information = Buffer(sizeof(ComputeShaderDataStructures::MeshInformation) * max_num_meshes, NULL);
		this->cluster_information = Buffer(sizeof(ComputeShaderDataStructures::ClusterInfo) * max_num_clusters, NULL);
		this->transformations = Buffer(sizeof(glm::mat4) * max_num_meshes, NULL);
		this->material_ids = Buffer(sizeof(uint32_t) * max_num_meshes, NULL);
		
		this->indirect_draw_info = Buffer(sizeof(ComputeShaderDataStructures::IndirectDrawInfo) * max_num_clusters, NULL);

		this->mesh_buffers_info.reserve(max_num_meshes);
		this->id_to_buffer_info.reserve(max_num_meshes);
	}

	std::optional<uint32_t> add_mesh(const Vertex3* vertex_data, size_t num_vertices, const unsigned int* indices_data, size_t num_indices) {
		size_t num_triangles_in_mesh = num_indices / 3;
		unsigned int num_add_indices = (num_triangles_per_cluster - (num_triangles_in_mesh % num_triangles_per_cluster)) * 3;
		
		size_t final_num_indices = num_indices + num_add_indices;
		size_t final_num_vertices = num_vertices + (num_add_indices != 0);

		size_t mesh_buffer_info_index = add_mesh_store_info(final_num_vertices * sizeof(Vertex3), final_num_indices * sizeof(unsigned int));
		auto mesh_store_info = mesh_buffers_info[mesh_buffer_info_index];

		this->vertices.modify(vertex_data, num_vertices * sizeof(Vertex3), mesh_store_info.vertices_offset);
		this->indices.modify(indices_data, num_indices * sizeof(unsigned int), mesh_store_info.indices_offset);

		if (num_add_indices != 0) {
			this->vertices.modify(&NaN_Vertex, sizeof(Vertex3), mesh_store_info.vertices_offset + num_vertices * sizeof(Vertex3));

			std::vector<unsigned int> extra_indices(num_add_indices, num_vertices);
			this->indices.modify(
				extra_indices.data(),
				num_add_indices * sizeof(unsigned int),
				mesh_store_info.indices_offset + num_indices * sizeof(unsigned int));
		}

		ComputeShaderDataStructures::ClusterInfo same_inf_each_cluster;
		same_inf_each_cluster.mesh_id = mesh_store_info.mesh_index;
		same_inf_each_cluster.num_triangles = num_triangles_per_cluster;
		std::vector<ComputeShaderDataStructures::ClusterInfo> cluster_inf(final_num_indices / (3 * num_triangles_per_cluster), same_inf_each_cluster);
		
		this->cluster_information.modify(cluster_inf.data(), cluster_inf.size() * sizeof(ComputeShaderDataStructures::ClusterInfo), mesh_store_info.clusters_offset);
		
		this->num_clusters += cluster_inf.size();
		this->num_meshes += 1;

		//mesh_index is the mesh id
		this->id_to_buffer_info[mesh_store_info.mesh_index] = mesh_buffer_info_index;


		////todo REMOVE when done testing
		//glm::mat4 wowtransform = glm::translate(glm::mat4(1), glm::vec3(0.));
		//
		//this->transformations.modify()

		return mesh_store_info.mesh_index;
	}

	const Buffer& get_indirect_draw_buffer()
	{
		this->create_indirect_draw_buffer();
		return this->indirect_draw_info;
	}

	void compute_indirect_draw_buffer()
	{
		culling_compute.bind_dispatch(this->num_clusters, 1, 1);
	}

	void create_indirect_draw_buffer()
	{
		//this is just a function created for testing.
		struct indirect_draw_struct
		{
			unsigned int  count;			// would equal number of triangles per cluster * 3
			unsigned int  instanceCount;	// setting it to 1 for now..
			unsigned int  firstIndex;		// number of indices before the first one of this cluster regardless of which mesh it belongs to
			unsigned int  baseVertex;		// number of vertices before the first one of cluster's mesh
			unsigned int  baseInstance;		// set it to mesh index for now..
		};

		std::vector<indirect_draw_struct> buff_data;
		buff_data.reserve(this->num_clusters);

		size_t cluster_counter = 0;

		for (const auto& mesh_buf_info : this->mesh_buffers_info) {
			unsigned int num_clusters_in_curr = mesh_buf_info.indices_size/ (sizeof(unsigned int) * 3 * num_triangles_per_cluster);
			for (unsigned int i = 0; i < num_clusters_in_curr; ++i) {
				indirect_draw_struct curr_inf;
				curr_inf.count = num_triangles_per_cluster * 3;
				curr_inf.instanceCount = 1;
				curr_inf.firstIndex = cluster_counter * num_triangles_per_cluster * 3;
				curr_inf.baseVertex = mesh_buf_info.vertices_offset / sizeof(Vertex3);
				curr_inf.baseInstance = mesh_buf_info.mesh_index;
				
				buff_data.push_back(curr_inf);
				cluster_counter += 1;
			}
		}

		this->indirect_draw_info.modify(buff_data.data(), buff_data.size() * sizeof(indirect_draw_struct), 0);
	}

	void set_instance(uint32_t static_mesh_id, glm::mat4 transform, uint32_t material_id) {
		//todo
		auto req_mesh_index = this->mesh_buffers_info[this->id_to_buffer_info[static_mesh_id]].mesh_index;
		this->transformations.modify(
			glm::value_ptr(transform),
			sizeof(glm::mat4),
			sizeof(glm::mat4) * req_mesh_index
		);
		this->material_ids.modify(
			&material_id,
			sizeof(uint32_t),
			sizeof(uint32_t) * req_mesh_index
		);
	}

	void draw_alone(uint32_t static_mesh_id) const {
		//this is only supposed to handle the draw call nothing else including transformation
		const auto& mesh_buff_info = this->mesh_buffers_info[static_mesh_id];
		VAO.draw(mesh_buff_info.indices_size / sizeof(unsigned int), mesh_buff_info.indices_offset / sizeof(unsigned int));
	}

	void bind_vao() const {
		VAO.bind();
	}

	void multi_draw() const {
		this->VAO.multi_draw_indirect(this->num_clusters, 0, 0);
	}

	void remove_mesh(unsigned int static_mesh_id) {
		//todo do this
	}

	const Buffer& get_transformation_buff() const {
		return this->transformations;
	}

	const Buffer& get_material_ids_buff() const {
		return this->material_ids;
	}

	const Buffer& get_indirect_draw_buff() const {
		return this->indirect_draw_info;
	}

};

/*
class MultiStaticMesh_OLD
{
private:
	BufferMulti indices_multi;
	BufferMulti vertices_multi;
	unsigned int num_meshes = 0;
	VertexArray VAO;
	Buffer indirect_draw_buffer;
	Buffer mesh_information_buffer;
	BufferMulti cluster_information_buffer;

	Buffer transform_matrices_buffer;
	Buffer material_ids_buffer;

	unsigned int base_instance_vertex_attribute_index = 3;
	unsigned int num_clusters = 0;

	const unsigned int num_triangles_per_cluster = 128;
	const unsigned char indices_type_size = 4;
	const unsigned short per_vertex_size = sizeof(float) * 8;

	ComputeShader culling_compute = ComputeShader("src\\Graphics\\CullClustersCompute.glsl");

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
	static const int MAX_NUM_MESHES = 1000;

	MultiStaticMesh_OLD(unsigned int num_triangles_per_cluster, unsigned char indices_type_size, unsigned short per_vertex_size, size_t max_size_vertex_buff, size_t max_size_index_buff) :
		num_triangles_per_cluster(num_triangles_per_cluster), indices_type_size(indices_type_size), per_vertex_size(per_vertex_size)
	{
		vertices_multi = BufferMulti(max_size_vertex_buff);
		indices_multi = BufferMulti(max_size_index_buff);
		VAO = VertexArray(vertices_multi.get_buffer(), indices_multi.get_buffer(), get_vertex_attribs());
		//glVertexAttribDivisor(3, 1);
		indirect_draw_buffer = Buffer(20 * (max_size_index_buff / (3 * 128)) * 8, NULL);
		mesh_information_buffer = Buffer(MAX_NUM_MESHES * 8, NULL);
		cluster_information_buffer = BufferMulti((max_size_index_buff / (3 * 128)) * 8);

		transform_matrices_buffer = Buffer(sizeof(glm::mat4) * 1000, NULL, GL_SHADER_STORAGE_BUFFER, 0, GL_STREAM_DRAW);
		material_ids_buffer = Buffer(sizeof(unsigned int) * 1000, NULL, GL_SHADER_STORAGE_BUFFER, 0, GL_STREAM_DRAW);

		culling_compute.add_ssbo_block("ClustersInfo");
		culling_compute.add_ssbo_block("MeshInfo");
		cluster_information_buffer.get_buffer().bind_base(GL_SHADER_STORAGE_BUFFER, 4);
		mesh_information_buffer.bind_base(GL_SHADER_STORAGE_BUFFER, 5);
		indirect_draw_buffer.bind_base(GL_SHADER_STORAGE_BUFFER, 3);
	}
	void multi_draw() const
	{
		//views_buff.bind_base(GL_SHADER_STORAGE_BUFFER, 6);	//avoid putting this here
		VAO.multi_draw_indirect(num_clusters, 0, 0);
	}
	const Buffer& get_indirect_draw_buffer() const
	{
		return indirect_draw_buffer;
	}

	void compute_indirect_draw_buffer()
	{
		culling_compute.bind_dispatch(this->num_clusters, 1, 1);
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

			if ((indices_offsets[i + 1] - indices_offsets[i]) / indices_type_size >= num_triangles_per_cluster * 3)
			{
				for (unsigned int x = 0; x < (indices_offsets[i + 1] - indices_offsets[i]) / indices_type_size; x += num_triangles_per_cluster * 3)
				{
					indirect_draw_struct curr;
					curr.count = num_triangles_per_cluster * 3;
					curr.instanceCount = 1;
					curr.baseVertex = baseVertex;
					curr.firstIndex = indices_offsets[i] / indices_type_size + x;
					std::cout << "\n" << curr.firstIndex << "\n";
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
	std::pair<VertexArray, long long> add_mesh(
		const void* vertices_data,
		size_t vertices_data_size,
		const void* indices_data,
		size_t indices_data_size,
		unsigned int num_triangles_actual,
		std::vector<VertexAttribData> attribs
	);

	void remove_mesh(VertexArray& vao) {
		//todo
	}
	
	void debug_output_indirect_draw(std::string filename)
	{
		std::ofstream file(filename);
		if (!file.is_open())
		{
			std::cerr << "could not open file: " << filename;
			exit(20);
		}
		unsigned int* buff_data = new unsigned int[num_clusters * 5];
		indirect_draw_buffer.access(buff_data, num_clusters * 5 * sizeof(unsigned int), 0);
		file << "count, instance_count, first_index, base_vertex, base_instance" << std::endl;
		for (unsigned int i = 0; i < num_clusters * 5; i += 5)
		{
			for (int x = 0; x < 5; ++x)
			{
				file << buff_data[i + x] << ',';
			}
			file << std::endl;
		}

		file << std::endl;

		file << "num_instances, base_vertex" << std::endl;
		mesh_information_buffer.access(buff_data, num_meshes * sizeof(unsigned int) * 2, 0);
		for (unsigned int i = 0; i < num_meshes * 2;i += 2)
		{
			file << buff_data[i] << ',' << buff_data[i + 1] << std::endl;
		}

		file << std::endl;

		file << "mesh_id, num_triangles" << std::endl;

		delete[] buff_data;
		buff_data = new unsigned int[num_clusters * 2];
		cluster_information_buffer.get_buffer().access(buff_data, sizeof(unsigned int) * num_clusters * 2, 0);
		for (unsigned int i = 0; i < num_clusters * 2;i += 2)
		{
			file << buff_data[i] << ',' << buff_data[i + 1] << std::endl;
		}

		delete[] buff_data;

		file.close();

	}

	void set_transform(unsigned int index, glm::mat4 transform)
	{
		transform_matrices_buffer.modify(&transform, sizeof(glm::mat4), index * sizeof(glm::mat4));
	}
	void set_transform(MeshStatic* mesh, glm::mat4 transform)
	{
		transform_matrices_buffer.modify(glm::value_ptr(transform), sizeof(glm::mat4), vertices_multi.get_object_index(mesh->get_vao()) * sizeof(glm::mat4));
	}
	void set_mat_id(MeshStatic* mesh, unsigned int  mat_id)
	{
		material_ids_buffer.modify(&mat_id, sizeof(unsigned int), vertices_multi.get_object_index(mesh->get_vao()) * sizeof(unsigned int));
	}

	Buffer get_transform_buffer() const
	{
		return transform_matrices_buffer;
	}

	inline unsigned int get_num_triangles_per_cluster() const { return num_triangles_per_cluster; }
};
*/

class HigherGraphics
{
public:
};

const float cube_vertices[8 * 36] = {
    // positions          // normals           // texture coords
    -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 0.0f,  0.0f,  0.5f,  -0.5f,
    -0.5f, 0.0f,  0.0f,  -1.0f, 1.0f,  0.0f,  0.5f,  0.5f,  -0.5f, 0.0f,
    0.0f,  -1.0f, 1.0f,  1.0f,  0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f,
    1.0f,  1.0f,  -0.5f, 0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f, 0.0f,  1.0f,
    -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f, 0.0f,  0.0f,

    -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.5f,  -0.5f,
    0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,  0.5f,  0.5f,  0.5f,  0.0f,
    0.0f,  1.0f,  1.0f,  1.0f,  0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
    1.0f,  1.0f,  -0.5f, 0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
    -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

    -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  1.0f,  0.0f,  -0.5f, 0.5f,
    -0.5f, -1.0f, 0.0f,  0.0f,  1.0f,  1.0f,  -0.5f, -0.5f, -0.5f, -1.0f,
    0.0f,  0.0f,  0.0f,  1.0f,  -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,
    0.0f,  1.0f,  -0.5f, -0.5f, 0.5f,  -1.0f, 0.0f,  0.0f,  0.0f,  0.0f,
    -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,  1.0f,  0.0f,

    0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.5f,  0.5f,
    -0.5f, 1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  0.5f,  -0.5f, -0.5f, 1.0f,
    0.0f,  0.0f,  0.0f,  1.0f,  0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,
    0.0f,  1.0f,  0.5f,  -0.5f, 0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
    0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

    -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  0.0f,  1.0f,  0.5f,  -0.5f,
    -0.5f, 0.0f,  -1.0f, 0.0f,  1.0f,  1.0f,  0.5f,  -0.5f, 0.5f,  0.0f,
    -1.0f, 0.0f,  1.0f,  0.0f,  0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,
    1.0f,  0.0f,  -0.5f, -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,  0.0f,  1.0f,

    -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.5f,  0.5f,
    -0.5f, 0.0f,  1.0f,  0.0f,  1.0f,  1.0f,  0.5f,  0.5f,  0.5f,  0.0f,
    1.0f,  0.0f,  1.0f,  0.0f,  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    1.0f,  0.0f,  -0.5f, 0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
    -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,  0.0f,  1.0f};

inline MeshData<Vertex3> get_cube() {
  MeshData<Vertex3> mesh_data;
  mesh_data.vertices.reserve(36);
  mesh_data.indices.reserve(36);
  Vertex3 curr;
  for (unsigned int i = 0; i < 36; ++i) {
    curr.pos = glm::vec3(cube_vertices[i * 8], cube_vertices[i * 8 + 1],
                         cube_vertices[i * 8 + 2]);
    curr.norm =
        glm::vec3(cube_vertices[3 + i * 8], cube_vertices[3 + i * 8 + 1],
                  cube_vertices[3 + i * 8 + 2]);
    curr.uv = glm::vec2(cube_vertices[6 + i * 8], cube_vertices[6 + i * 8 + 1]);
    mesh_data.vertices.push_back(curr);
    mesh_data.indices.push_back(i);
  }
  return mesh_data;
}

// TODO MAKE A GLSL SHADER PARSER TO AUTOMATICALLY DETECT ALL UNIFORMS IN A
// SHADER 	use glGetProgramInterfaceiv etc

/*
 * checklist:
 *	1. a)make texture constructors work better
 *	1. b)	make texture parameters changeable
 *	2.)	make framebuffer depth attachment less easy to fail with
 *	3.)	make fragment shader changeable for debug texture draw
 *	4.) make compute shader have all features of normal pipelines
 *	5.) seperate graphics and context creation
 *	6.) if you want to, make a shadow demo
 */

/*
 * checklist 2:
 *	1.) make all mesh types store the data on the cpu using the same class
 *	2.) make buffers take data using templates not void* pointers
 */