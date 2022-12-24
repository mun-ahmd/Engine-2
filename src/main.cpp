#include "glad/glad.h"
#include <string>
#include <unordered_map>
#define MAIN_DEFAULT
#ifdef MAIN_DEFAULT

#include <iostream>
#include <chrono>
#include <functional>
#include <filesystem>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/norm.hpp"

#include "Graphics/Graphics_2.h"
#include "Graphics/HigherGraphics_2.h"

#include "Mesh/Mesh.h"
#include "Asset/Model.h"
#include "Asset/TextureAsset.h"
#include "Scene/SceneIO.h"

#include "Utils/View.h"
#include "ECS/ECS.hpp"

#include "Graphics/RenderPass.h"

//TODO MAKE A GLSL SHADER PARSER TO AUTOMATICALLY DETECT ALL UNIFORMS IN A SHADER
//	use glGetProgramInterfaceiv etc

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

const float cube_vertices[8 * 36] = {
	// positions          // normals           // texture coords
	-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
	 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
	 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
	 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
	-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
	-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

	-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
	 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
	 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
	 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
	-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
	-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

	-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
	-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
	-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
	-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
	-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
	-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

	 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
	 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
	 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
	 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
	 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
	 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
	 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
	 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
	 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
	-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

	-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
	 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
	 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
	 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
	-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
	-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
};

MeshData<Vertex3> get_cube() {
	MeshData<Vertex3> mesh_data;
	mesh_data.vertices.reserve(36);
	mesh_data.indices.reserve(36);
	Vertex3 curr;
	for (unsigned int i = 0; i < 36; ++i) {
		curr.pos = glm::vec3(cube_vertices[i * 8], cube_vertices[i * 8 + 1], cube_vertices[i * 8 + 2]);
		curr.norm = glm::vec3(cube_vertices[3 + i * 8], cube_vertices[3 + i * 8 + 1], cube_vertices[3 + i * 8 + 2]);
		curr.uv = glm::vec2(cube_vertices[6 + i * 8], cube_vertices[6 + i * 8 + 1]);
		mesh_data.vertices.push_back(curr);
		mesh_data.indices.push_back(i);
	}
	return mesh_data;
}

std::shared_ptr<MeshStatic> mesh_static_loader(std::string filedir) {
	assert(filedir.substr(filedir.find_last_of('.'), filedir.size() - 1) == ".twomesh");

	std::ifstream file(filedir, std::ios::binary);
	if (file.is_open() == false) {
		//todo error
	}
	char signature[] = "twomesh";
	file.read(signature, strlen(signature));
	unsigned short version[2] = { 0,0 };
	file.read((char*)version, sizeof(unsigned short) * 2);
	assert(std::string(signature) == "TWOMESH" && version[0] == 1);

	unsigned int num_vertices = 0;
	file.read((char*)&num_vertices, sizeof(unsigned int));
	unsigned int num_triangles = 0;
	file.read((char*)&num_triangles, sizeof(unsigned int));

	std::vector<Vertex3> vertices;
	vertices.resize(num_vertices);
	file.read((char*)&vertices[0], vertices.size() * sizeof(Vertex3));

	std::vector<unsigned int> indices;
	indices.resize(static_cast<size_t>(num_triangles) * 3);
	file.read((char*)indices.data(), indices.size() * sizeof(unsigned int));

	file.close();
	return std::make_shared<MeshStatic>(MeshData(vertices, indices));
}

void mesh_static_unloader(std::shared_ptr<MeshStatic> ptr) {
	ptr->unload();
}

typedef Asset<MeshStatic, mesh_static_loader, mesh_static_unloader> MeshStaticAsset;

glm::mat4 compute_dir_lightspace(const glm::mat4& proj, const glm::mat4& view, const glm::vec3& lightdir) {
	glm::vec4 left(std::numeric_limits<float>::max()),
		right(std::numeric_limits<float>::min()),
		bottom(std::numeric_limits<float>::max()),
		top(std::numeric_limits<float>::min()),
		near(std::numeric_limits<float>::max()),
		far(std::numeric_limits<float>::min());
	auto inv_view = glm::inverse(proj*view);
	glm::vec4 unitCube[2][2][2];
	for(int x = 0; x < 2; ++x)
		for (int y = 0; y < 2; ++y)
			for (int z = 0; z < 2; ++z) {
				unitCube[x][y][z]  = glm::vec4(glm::vec3((float)x, (float)y, (float)z) * 2.f - 1.f, 1.0);
				unitCube[x][y][z] = inv_view * unitCube[x][y][z];
				unitCube[x][y][z] /= unitCube[x][y][z].w;
				//std::cout << unitCube[x][y][z].x << ", " << unitCube[x][y][z].y << ", " << unitCube[x][y][z].z << std::endl;
			}
	for (int x = 0; x < 2; ++x)
		for (int y = 0; y < 2; ++y)
			for (int z = 0; z < 2; ++z) {
				auto c = unitCube[x][y][z];
				right = c.x > right.x ? c : right;
				left = c.x < left.x ? c : left;
				top = c.y > top.y ? c : top;
				bottom = c.y < bottom.y ? c : bottom;
				near = c.z < near.z ? c : near;
				far = c.z > far.z ? c : far;
			}
	// return glm::ortho(left.x, right.x, bottom.y, top.y, near.z, far.z);
	return proj*view;
}
#include <filesystem>
int main() {

	// int maxBlocks;
	// glGetIntegerv(GL_MAX_VERTEX_UNIFORM_BLOCKS, &maxBlocks);
	// std::cout << "\nMax vertex uniform blocks: " << maxBlocks;
	// glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS, &maxBlocks);
	// std::cout << "\nMax fragment uniform blocks: " << maxBlocks;
	// glGetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, &maxBlocks);
	// std::cout << "\nMax vertex shader storage blocks: " << maxBlocks;
	// glGetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, &maxBlocks);
	// std::cout << "\nMax fragment shader storage blocks: " << maxBlocks;
	// glGetIntegerv(GL_MAX_COMPUTE_UNIFORM_BLOCKS, &maxBlocks);
	// std::cout << "\nMax compute uniform blocks: " << maxBlocks;
	// glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, &maxBlocks);
	// std::cout << "\nMax compute shader storage blocks: " << maxBlocks;
	// return 0;

	std::cout << std::filesystem::current_path();	
	GLFWwindow* window = Graphics::InitiateGraphicsLib({ "GL_ARB_bindless_texture" });
	static int width, height;
	glfwGetWindowSize(window, &width, &height);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	CamHandler cam(window);

	auto default_fbo = Framebuffer::default_framebuffer(width, height);

	typedef FramebufferAttachment FBOattach;
	
	//Graphics resources
	struct {
		// Pipeline geometry = Pipeline(R"(src/Graphics/Shaders/newDrawVert.glsl)", R"(src/Graphics/Shaders/newDrawFrag.glsl)");
		Pipeline texture_to_depth_attachment = Pipeline(R"(src/Graphics/Shaders/FullScreenVert.glsl)", R"(src/Graphics/Shaders/TextureToDepthFrag.glsl)");
		Pipeline material = Pipeline(R"(src/Graphics/Shaders/MaterialShaderVert.glsl)", R"(src/Graphics/Shaders/MaterialShader.glsl)");
		Pipeline shadow_mapper = Pipeline(R"(src/Graphics/Shaders/ShadowVert.glsl)", R"(src/Graphics/Shaders/ShadowFrag.glsl)");

	} pipes;
	struct {
		Buffer proj_view;
		Buffer models;
		Buffer material_ids;

		Buffer materials;
		Buffer g_buffer_handles;
		Buffer active_material_id;
	} buffs;
	struct {
		// Texture_2D g_pos = Texture_2D(width, height, GL_RGB32F);
		// Texture_2D g_norm = Texture_2D(width, height, GL_RGB32F);
		// Texture_2D material_depth = Texture_2D(width, height, GL_RGB32F);

		Texture_2D material_pass_out = Texture_2D(width, height, GL_RGBA);

		Texture_2D shadow_map = Texture_2D(width, height, GL_DEPTH_COMPONENT32);
	} textures;
	struct {
		// Renderbuffer geometry_pass_depth = Renderbuffer(width, height, GL_DEPTH24_STENCIL8);

		Renderbuffer material_pass_depth = Renderbuffer(width, height, GL_DEPTH_COMPONENT32F);
	} renderbuffs;

	//Framebuffers
	// Framebuffer g_buffer(width, height,
	// 	{ FBOattach(textures.g_pos, true),
	// 		FBOattach(textures.g_norm, true),
	// 		FBOattach(textures.material_depth, true) },
	// 	FBOattach(renderbuffs.geometry_pass_depth, false), DepthStencilAttachType::BOTH_DEPTH_STENCIL
	// );
	ECSmanager ecs;
	ecs.register_component<Buffer>();
	ecs.register_component<Pipeline>();
	ecs.register_component<Framebuffer>();
	ecs.register_component<Texture_2D>();
	std::unordered_map<std::string, Entity> renderer_entities;

	Framebuffer material_pass_fbo(width, height,
		{ FBOattach(textures.material_pass_out, true) },
		FBOattach(renderbuffs.material_pass_depth, false), DepthStencilAttachType::ONLY_DEPTH
	);

	Framebuffer shadow_pass_fbo(width, height,
		{},
		FBOattach(textures.shadow_map, true), DepthStencilAttachType::ONLY_DEPTH);

	//THIS DEMO WILL HAVE 2 CUBES OF DIFFERENT COLORS
	float near_far[2] = { 0.001f, 100.0f };
	glm::mat4 projection_mat = glm::perspective(45.f, (float)width / height, near_far[0], near_far[1]);
	//constexpr int num_demo_meshes = 2;
	std::vector<glm::vec4> materials = { glm::vec4(0.0,1.0,1.0,1.0), glm::vec4(1.0,0.0,0.0,1.0) };
	struct DemoMesh {
		MeshStaticAsset mesh;
		glm::mat4 model;
		uint32_t mat_id;
		DemoMesh(MeshStaticAsset mesh, glm::mat4 model, uint32_t mat_id) : mesh(mesh), model(model), mat_id(mat_id) {}
	};
	std::vector<DemoMesh> demo_meshes;
	for (const auto& file : std::filesystem::directory_iterator("Example/birchTwoMesh")) {
		std::string filepath_string = file.path().string();
		constexpr glm::vec4 leafColor = (glm::vec4(250, 187, 148, 255) / 255.f);
		if (filepath_string == R"(Example/birchTwoMesh/2_Birch_Leaf_Autumn_1.twomesh)")
			materials.push_back(leafColor);
		else if (filepath_string == R"(Example/birchTwoMesh/0_Leaves_Mesh.003.twomesh)")
			materials.push_back(leafColor);
		else if (filepath_string == R"(Example/birchTwoMesh/1_Material.001.twomesh)")
			materials.push_back(leafColor);
		else if (filepath_string == R"(Example/birchTwoMesh/3_Bark_and_Branches_Mesh.005.twomesh)")
			materials.push_back(glm::vec4(175, 157, 153, 255)/255.f);
		else {
			std::cout << std::endl << filepath_string << std::endl;
			materials.push_back(glm::vec4(255, 255, 255, 255) / 255.f);
		}
		demo_meshes.push_back(DemoMesh(MeshStaticAsset(filepath_string.c_str()), glm::mat4(1), materials.size() - 1));
		demo_meshes.back().mesh.get_asset();
	}

	//FURTHER INITIALIZATIONS

		//making Textures resident
	// textures.material_depth.make_handle_resident();
	// textures.g_pos.make_handle_resident();
	// textures.g_norm.make_handle_resident();
	textures.shadow_map.make_handle_resident();

	//Buffers initialization
	buffs.proj_view = Buffer(sizeof(glm::mat4) * 2, NULL);
	buffs.proj_view.modify(glm::value_ptr(projection_mat), sizeof(glm::mat4), 0);

	buffs.models = Buffer(sizeof(glm::mat4) * demo_meshes.size(), NULL);

	buffs.material_ids = Buffer(sizeof(uint32_t) * demo_meshes.size(), NULL);

	buffs.materials = Buffer(sizeof(glm::vec4) * materials.size(), NULL);
	buffs.materials.modify(materials.data(), sizeof(glm::vec4) * materials.size(), 0);

	Buffer indirect_draw_buffer = MeshStatic::get_static_meshes_holder().get_indirect_draw_buffer();
    renderer_entities["indirect_draw_buf"] = ecs.new_entity(indirect_draw_buffer);
    renderer_entities["proj_view_buf"] = ecs.new_entity(buffs.proj_view);
    renderer_entities["models_buf"] = ecs.new_entity(buffs.models);
    renderer_entities["material_ids_buf"] = ecs.new_entity(buffs.material_ids);
	GeometryPass geometry_pass(width, height);
	geometry_pass.init(ecs, renderer_entities);

	buffs.g_buffer_handles = Buffer(sizeof(uint64_t) * 2, NULL);
	std::array<uint64_t, 2> gbuff_handles = { ecs.get_component<Texture_2D>(renderer_entities["g_pos_tex"])->get_handle(), ecs.get_component<Texture_2D>(renderer_entities["g_norm_tex"])->get_handle() };
	buffs.g_buffer_handles.modify(gbuff_handles.data(), sizeof(uint64_t) * 2, 0);

	buffs.active_material_id = Buffer(sizeof(uint32_t), NULL);

	for (int i = 0; i < demo_meshes.size(); ++i) {
		buffs.models.modify(glm::value_ptr(demo_meshes[i].model), sizeof(glm::mat4), sizeof(glm::mat4) * i);
		uint32_t actual_mat_id = demo_meshes[i].mat_id + 1;
		buffs.material_ids.modify(&actual_mat_id, sizeof(uint32_t), sizeof(uint32_t) * i);
	}

	//PIPELINES SETUP
	pipes.texture_to_depth_attachment.change_depth_func(GL_ALWAYS);
	pipes.material.change_depth_func(GL_EQUAL);

	//uniforms & ssbos
	// pipes.geometry.add_pipeline_uniform_block("Matrices");
	// pipes.geometry.bind_pipeline_uniform_block("Matrices", 1);
	// pipes.geometry.add_pipeline_ssbo_block("Transformations");
	// pipes.geometry.bind_pipeline_ssbo_block("Transformations", 2);
	// pipes.geometry.add_pipeline_ssbo_block("MaterialIds");
	// pipes.geometry.bind_pipeline_ssbo_block("MaterialIds", 3);

	pipes.shadow_mapper.add_pipeline_uniform_block("Matrices");
	pipes.shadow_mapper.bind_pipeline_uniform_block("Matrices", 1);
	pipes.shadow_mapper.add_pipeline_ssbo_block("Transformations");
	pipes.shadow_mapper.bind_pipeline_ssbo_block("Transformations", 2);

	pipes.texture_to_depth_attachment.set_uniform<uint64_t>(
		pipes.texture_to_depth_attachment.get_uniform_loc("depth_texture"),
		ecs.get_component<Texture_2D>(renderer_entities["material_depth_tex"])->get_handle()
		);

	pipes.material.add_pipeline_uniform_block("MaterialID");
	pipes.material.bind_pipeline_uniform_block("MaterialID", 2);
	pipes.material.add_pipeline_uniform_block("gbuffer");
	pipes.material.bind_pipeline_uniform_block("gbuffer", 3);
	pipes.material.add_pipeline_ssbo_block("Materials");
	pipes.material.bind_pipeline_ssbo_block("Materials", 4);
	pipes.material.set_uniform<uint64_t>(pipes.material.get_uniform_loc("shadow_map"), textures.shadow_map.get_handle());


	//binding buffers to shader blocks
	auto geometry = ecs.get_component<Pipeline>(renderer_entities["geometry_pipe"]);
	buffs.proj_view.bind_base(GL_UNIFORM_BUFFER,
		geometry->get_pipeline_uniform_block_binding("Matrices"));
	buffs.models.bind_base(GL_SHADER_STORAGE_BUFFER,
		geometry->get_pipeline_ssbo_block_binding("Transformations"));
	buffs.material_ids.bind_base(GL_SHADER_STORAGE_BUFFER,
		geometry->get_pipeline_ssbo_block_binding("MaterialIds"));

	buffs.materials.bind_base(GL_SHADER_STORAGE_BUFFER,
		pipes.material.get_pipeline_ssbo_block_binding("Materials"));
	buffs.active_material_id.bind_base(GL_UNIFORM_BUFFER,
		pipes.material.get_pipeline_uniform_block_binding("MaterialID"));
	buffs.g_buffer_handles.bind_base(GL_UNIFORM_BUFFER,
		pipes.material.get_pipeline_uniform_block_binding("gbuffer"));


	double delta_time = 0.0;

	glBindTexture(GL_TEXTURE_2D, 0);

	Graphics::clear_color(0.3, 0.3, 0.3, 1);

	DebugTextureDrawer tex_drawer;
	tex_drawer.set_uniform_vec<float, 2>("near_far", &near_far[0]);
	//todo make better ways
	glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glm::vec3 LIGHTPOS = glm::vec3(0.5, 1, 1) * 10.f;
	glm::mat4 LIGHTVIEW = glm::lookAt(LIGHTPOS, glm::vec3(0), glm::vec3(0, 1, 0));
	pipes.material.set_uniform_vec<3,float>(pipes.material.get_uniform_loc("lightPos"), (float*)&LIGHTPOS);
	
	while (!glfwWindowShouldClose(window) && !(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)) {
		double start_time = glfwGetTime();
		//update view matrix
		cam.moveAround(delta_time); cam.lookAround();
		glm::vec3 campos = cam.get_pos();
		pipes.material.set_uniform_vec<3>(pipes.material.get_uniform_loc("camera_pos"), glm::value_ptr(campos));
		glm::mat4 curr_view = cam.getView();

		shadow_pass_fbo.bind(GL_FRAMEBUFFER);
		shadow_pass_fbo.clear(GL_DEPTH_BUFFER_BIT);
		pipes.shadow_mapper.bind();
		//todo do in better ways
		buffs.proj_view.modify(glm::value_ptr(LIGHTVIEW), sizeof(glm::mat4), sizeof(glm::mat4));
		glm::mat4 lightspacetrans = compute_dir_lightspace(projection_mat, curr_view, LIGHTPOS);
		pipes.shadow_mapper.set_uniform_mat<4, 4>(pipes.shadow_mapper.get_uniform_loc("lightspace"), glm::value_ptr(lightspacetrans));
		ecs.get_component<Buffer>(renderer_entities["indirect_draw_buf"])->bind(GL_DRAW_INDIRECT_BUFFER);
		MeshStatic::get_static_meshes_holder().multi_draw();

		buffs.proj_view.modify(glm::value_ptr(curr_view), sizeof(glm::mat4), sizeof(glm::mat4));

		// g_buffer.bind(GL_FRAMEBUFFER);
		// g_buffer.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// pipes.geometry.bind();

		// indirect_db.bind(GL_DRAW_INDIRECT_BUFFER);
		// MeshStatic::get_static_meshes_holder().multi_draw();
		geometry_pass.execute(ecs, renderer_entities);

		material_pass_fbo.bind(GL_FRAMEBUFFER);
		material_pass_fbo.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		pipes.texture_to_depth_attachment.bind();
		glDrawArrays(GL_TRIANGLES, 0, 3);

		pipes.material.bind();
		pipes.material.set_uniform_mat<4, 4>(pipes.material.get_uniform_loc("lightspace"), glm::value_ptr(lightspacetrans));
		for (uint32_t i = 0; i < materials.size(); ++i) {
			//update active mat id
			uint32_t material_id = i + 1;
			buffs.active_material_id.modify(&material_id, sizeof(uint32_t), 0);

			glDrawArrays(GL_TRIANGLES, 0, 3);
		}

		default_fbo.bind(GL_FRAMEBUFFER);
		default_fbo.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		tex_drawer(textures.material_pass_out);

		glfwSwapBuffers(window);
		glfwPollEvents();
		delta_time = glfwGetTime() - start_time;
	}

	return 0;
}
#else


#include <iostream>
#include "glm/glm.hpp"
#include <chrono>

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/norm.hpp"

#include "Asset/Model.h"
#include "Utils/View.h"
#include "Graphics/Graphics_2.h"
#include "Graphics/cameraObj.h"
#include "Asset/TextureAsset.h"

#include "ECS/ECS.hpp"
#include "Graphics/HigherGraphics_2.h"

#include "Scene/SceneIO.h"

#include <filesystem>


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

		this->cam.mouseLookProcess(xOffset, yOffset,true);
	}

	glm::mat4 getView()
	{
		return this->cam.getViewMatrix();
	}

};



void printVEC3(glm::vec3& vec)
{
	std::cout << '(' << vec.x << ',' << vec.y << ',' << vec.z << ')' << std::endl;
}

void printPosition(Position& pos_)	{printVEC3(pos_.pos);}

glm::vec3 random_vec3() {
	std::srand(static_cast<unsigned int>(std::time(nullptr)));
	return glm::vec3((float)std::rand() / RAND_MAX, (float)std::rand() / RAND_MAX, (float)std::rand() / RAND_MAX);
}


std::shared_ptr<MeshStatic> mesh_static_loader(std::string filedir) {
	assert(filedir.substr(filedir.find_last_of('.'),filedir.size() - 1) == ".twomesh");
	
	std::ifstream file(filedir, std::ios::binary);
	if (file.is_open() == false) {
		//todo error
	}
	char signature[] = "twomesh";
	file.read(signature, strlen(signature));
	unsigned short version[2] = { 0,0 };
	file.read((char*)version, sizeof(unsigned short) * 2);
	assert(std::string(signature) == "TWOMESH" && version[0] == 1);

	unsigned int num_vertices = 0;
	file.read((char*)&num_vertices, sizeof(unsigned int));
	unsigned int num_triangles = 0;
	file.read((char*)&num_triangles, sizeof(unsigned int));

	std::vector<Vertex3> vertices;
	vertices.resize(num_vertices);
	file.read((char*)&vertices[0], vertices.size() * sizeof(Vertex3));

	std::vector<unsigned int> indices;
	indices.resize(static_cast<size_t>(num_triangles) * 3);
	file.read((char*)indices.data(), indices.size() * sizeof(unsigned int));

	file.close();
	return std::make_shared<MeshStatic>(MeshData(vertices, indices));
}

void mesh_static_unloader(std::shared_ptr<MeshStatic> ptr) {
	ptr->unload();
}

typedef Asset<MeshStatic, mesh_static_loader, mesh_static_unloader> MeshStaticAsset;


int main()
{
	
	//ECSmanager ecs;
	//ecs.register_component<Position>();
	//for (int i = 0; i < 100; ++i)
	//	ecs.new_entity(Position((float)i));
	//
	//ecs.register_system(
	//	[](ECSmanager& ecs)
	//	{
	//		float speed = 0.001f;
	//		View<Position> pos = ecs.get_components_of_type<Position>();
	//		for (uint32_t i = 0; i < pos.size(); ++i)
	//		{
	//			pos[i].pos += glm::vec3(speed);
	//		}
	//	});
	//ecs.register_system(
	//	[](ECSmanager& ecs)
	//	{
	//		ecs.get_components_of_type<Position>().iterate(printPosition);
	//	});

	////ecs.get_components_of_type<Position>().iterate(&printPosition);
	//std::cout << "\nEXECUTING SYSTEMS\n";

	//for (uint32_t i = 0; i < 100; ++i)
	//{
	//	std::cout << "\n\n\n\n" << i << "\n\n\n";
	//	ecs.execute_systems();	//to be called every frame
	//}
 //	std::cout << "\nSYSTEMS EXECUTED\n";
	//ecs.get_components_of_type<Position>().iterate(&printPosition);


	
	//float vertices_color[] = {
	//-0.5f, -0.5f, 0.0f, // left
	// 0.0f,1.0f,0.0f,
	// 0.5f, -0.5f, 0.0f, // right 
	// 0.0f,0.0f,1.0f,
	// 0.0f,  0.5f, 0.0f,  // top   
	// 1.0f,0.0f,0.0f
	//};

	GLFWwindow* window = Graphics::InitiateGraphicsLib({"GL_ARB_bindless_texture"});
	int window_height, window_width;
	glfwGetWindowSize(window, &window_width, &window_height);
	Framebuffer defaultfbo = Framebuffer::default_framebuffer(window_width,window_height);
	Pipeline pipe = Pipeline(
		R"(src/Graphics/newDrawVert.glsl)",
		R"(src/Graphics/newDrawFrag.glsl)");

	glm::mat4 projection_matrix = glm::perspective(
			glm::radians(45.0f),	//field of view = 45 degrees
			((float)window_width/ window_height),				//width over height of frustum	//I have set to window width over window height
			0.001f,					//z near
			3000.0f					// z far
		);
	glViewport(0, 0, window_width, window_height);

	CamHandler cam(window);
	double deltaTime = 0.0;
	double startTime = glfwGetTime();
	double mouseLastX = 0.0;
	double mouseLastY = 0.0;
	bool hasMouseMovedOnce = false;

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glEnable(GL_DEPTH_TEST);

	Buffer pvm_matrices = Buffer(sizeof(glm::mat4) * 3, NULL, GL_UNIFORM_BUFFER, false);
	pvm_matrices.modify(glm::value_ptr(projection_matrix), sizeof(glm::mat4), 0);

	pipe.add_pipeline_uniform_block("Matrices");
	pipe.bind_pipeline_uniform_block("Matrices", 2);
	pvm_matrices.bind_range(GL_UNIFORM_BUFFER, pipe.get_pipeline_uniform_block_binding("Matrices"), 0, sizeof(glm::mat4) * 2);


	ECSmanager ecs;
	ecs.register_component<MeshStatic*>();
	ecs.register_component<Position>();

	std::vector<MeshStaticAsset> treeModel;

	//unsigned char default_colour[3] = { 255,0,255 };
	//Material default_mat(
	//	Texture2DAsset(std::make_unique<Texture_2D>(Texture_2D(1, 1, GL_RGB, GL_UNSIGNED_BYTE, &default_colour[0], GL_RGB))),
	//	Texture2DAsset(std::make_unique<Texture_2D>(Texture_2D(1, 1, GL_RGB, GL_UNSIGNED_BYTE, &default_colour[0], GL_RGB))),
	//	Texture2DAsset(std::make_unique<Texture_2D>(Texture_2D(1, 1, GL_RGB, GL_UNSIGNED_BYTE, &default_colour[0], GL_RGB))),
	//	Texture2DAsset(std::make_unique<Texture_2D>(Texture_2D(1, 1, GL_RGB, GL_UNSIGNED_BYTE, &default_colour[0], GL_RGB))),
	//	Texture2DAsset(std::make_unique<Texture_2D>(Texture_2D(1, 1, GL_RGB, GL_UNSIGNED_BYTE, &default_colour[0], GL_RGB))),
	//	Texture2DAsset(std::make_unique<Texture_2D>(Texture_2D(1, 1, GL_RGB, GL_UNSIGNED_BYTE, &default_colour[0], GL_RGB))),
	//	Texture2DAsset(std::make_unique<Texture_2D>(Texture_2D(1, 1, GL_RGB, GL_UNSIGNED_BYTE, &default_colour[0], GL_RGB)))
	//);
	// TODO WORK ON THIS LATER

	std::vector<glm::vec4> material_data;

	for (const auto& file : std::filesystem::directory_iterator("Example/birchTwoMesh")) {
		std::string filepath_string = file.path().string();
		treeModel.push_back(MeshStaticAsset(filepath_string.c_str()));

		treeModel.back().get_asset();
		//treeModel.back().unload();
		MeshStatic::get_static_meshes_holder().set_instance(treeModel.back().get_asset()->get_static_mesh_id(), glm::mat4(1), material_data.size());
		material_data.push_back(glm::vec4(random_vec3(),1.0));
	}
	//MeshStaticAsset leaves("C:\\Users\\munee\\source\\repos\\Engine-2\\3DModelData\\birchTwoMesh\\0_Leaves_Mesh.003-0.twomesh");
	//leaves.get_asset()->add_instance(glm::vec3(0.0));

	MeshStatic::get_static_meshes_holder().get_transformation_buff().bind_base(GL_SHADER_STORAGE_BUFFER, 0);
	MeshStatic::get_static_meshes_holder().get_material_ids_buff().bind_base(GL_SHADER_STORAGE_BUFFER, 6);

	pipe.bind();


	unsigned char pixfnoa[3] = { 255,0,0 };
	Texture_2D sapdpia(1, 1, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, &pixfnoa[0]);
	sapdpia.make_handle_resident();
	int indwasda = 2;
	//pipe.add_sampler_uniform("albedo");
	//pipe.set_texture_handle("albedo", sapdpia);
	//glUniform1ui64ARB(glGetUniformLocation(pipe.get_program(), "albedo"), sapdpia.get_handle());

	MeshStatic::prepare_indirect_draw_buffer();
	pipe.bind();
	const Buffer& indirect_draw_buff = MeshStatic::get_static_meshes_holder().get_indirect_draw_buff();
	indirect_draw_buff.bind(GL_DRAW_INDIRECT_BUFFER);





	




	Framebuffer matfbo(window_width, window_height,
		{ FramebufferAttachment(Renderbuffer(window_width,window_height,GL_RGBA),true) },
		FramebufferAttachment(Renderbuffer(window_width, window_height, GL_DEPTH_COMPONENT32F), false),
		DepthStencilAttachType::ONLY_DEPTH);


	uint32_t shadow_width = window_width, shadow_height = window_height;
	Framebuffer shadowfbo(shadow_width, shadow_height,
		{},
		FramebufferAttachment(Texture_2D(shadow_width, shadow_height, GL_DEPTH_COMPONENT), false),
		DepthStencilAttachType::ONLY_DEPTH);
	Pipeline shadow_pipe(
		R"(src/Graphics/ShadowVert.glsl)",
		R"(src/Graphics/ShadowFrag.glsl)");
	shadow_pipe.add_pipeline_uniform_block("Matrices");
	shadow_pipe.bind_pipeline_uniform_block("Matrices", 2);

	Framebuffer otherfbo(window_width, window_height,
		{ FramebufferAttachment(Texture_2D(window_width,window_height,GL_RGB32F),true),
		  FramebufferAttachment(Texture_2D(window_width,window_height,GL_RGB32F),true),
		  FramebufferAttachment(Texture_2D(window_width,window_height,GL_RED),true) },
		FramebufferAttachment(Renderbuffer(window_width, window_height, GL_DEPTH24_STENCIL8), false),
		DepthStencilAttachType::BOTH_DEPTH_STENCIL);
	//uint32_t shadow_width = 512;
	//uint32_t shadow_height = 512;
	//Framebuffer shadowfbo(shadow_width, shadow_height, {FramebufferAttachment(Renderbuffer(shadow_width,shadow_height,GL_RED),true)}, FramebufferAttachment(Texture_2D(shadow_width, shadow_height, GL_DEPTH_COMPONENT32), false), DepthStencilAttachType::ONLY_DEPTH);



	otherfbo.bind(GL_FRAMEBUFFER);
	otherfbo.set_read_buffer(0);
	for (auto& _attachment : otherfbo.get_color_attachments()) {
		_attachment.texture.make_handle_resident();
	}
	uint64_t gbuffer_handles[2];
	for (int i = 0; i < 2; ++i) {
		gbuffer_handles[i] = otherfbo.get_color_attachments()[i].texture.get_handle();
	}

	Pipeline depthTexToBuffer(R"(src/Graphics/FullScreenVert.glsl)", R"(src/Graphics/TextureToDepthFrag.glsl)");

	glProgramUniformHandleui64ARB(depthTexToBuffer.get_program(),
		glGetAttribLocation(depthTexToBuffer.get_program(), "depth_texture"),
		otherfbo.get_color_attachments()[2].texture.get_handle());


	Pipeline materialPipe(R"(src/Graphics/MaterialShaderVert.glsl)", R"(src/Graphics/MaterialShader.glsl)");
	Buffer material_buff(material_data.size() * sizeof(glm::vec4), material_data.data());
	materialPipe.add_pipeline_ssbo_block("Materials");
	Buffer material_id(sizeof(uint32_t), NULL);
	materialPipe.add_pipeline_uniform_block("MaterialID");
	materialPipe.bind_pipeline_uniform_block("MaterialID", 1);
	material_id.bind_base(GL_UNIFORM_BUFFER, materialPipe.get_pipeline_uniform_block_binding("MaterialID"));

	Buffer gbuff_handles_buffer(sizeof(uint64_t) * 2, &gbuffer_handles[0]);
	materialPipe.add_pipeline_uniform_block("gbuffer");
	materialPipe.bind_pipeline_uniform_block("gbuffer", 10);
	gbuff_handles_buffer.bind_base(GL_UNIFORM_BUFFER, materialPipe.get_pipeline_uniform_block_binding("gbuffer"));

	shadowfbo.get_depth_stencil_attachment().texture.make_handle_resident();
	uint64_t shadowmap_handle = shadowfbo.get_depth_stencil_attachment().texture.get_handle();



	Buffer lightspacemat(sizeof(glm::mat4),NULL);
	materialPipe.add_pipeline_uniform_block("lightspace_mat");
	materialPipe.bind_pipeline_uniform_block("lightspace_mat", 8);
	lightspacemat.bind_base(GL_UNIFORM_BUFFER, materialPipe.get_pipeline_uniform_block_binding("lightspace_mat"));



	material_buff.bind_base(GL_SHADER_STORAGE_BUFFER, 7);

	glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);

	MeshStatic::get_static_meshes_holder().bind_vao();


	//store_scene("scene.json", &ecs);
	
	ecs.register_system([&cam,&projection_matrix,&pvm_matrices,&pipe, &otherfbo](ECSmanager& ecs_manager) 
		{
			//auto meshes = ecs_manager.get_components_of_type<MeshStatic*>();

			//auto positions = ecs_manager.get_components_of_type<Position>();
			pipe.bind();
			auto view_matrix = cam.getView();
			pvm_matrices.modify(glm::value_ptr(view_matrix), sizeof(glm::mat4), sizeof(glm::mat4));
			for (int i = 0; i < 1; ++i)
			{
				//pipe.set_texture_handle("albedo", mats[i]->get_handles()[0]);
				MeshStatic::get_static_meshes_holder().multi_draw();
				//meshes[i]->draw();
			}
		});

	while (!glfwWindowShouldClose(window))
	{
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);
		deltaTime = (double)glfwGetTime() - startTime;
		startTime = glfwGetTime();
		cam.lookAround();
		cam.moveAround(deltaTime);




		otherfbo.bind(GL_FRAMEBUFFER);

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		////model2_.draw();
		//model2_.draw();
		ecs.execute_systems();

		shadowfbo.bind(GL_FRAMEBUFFER);
		glClear(GL_DEPTH_BUFFER_BIT);
		shadow_pipe.bind();

		glm::mat4 light_view = glm::lookAt(glm::vec3(0.8, 0.45, 0.2) * 10.0f, glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));
		glm::mat4 lightspace = projection_matrix * light_view;
		lightspacemat.modify(glm::value_ptr(lightspace), sizeof(glm::mat4), 0);
		pvm_matrices.modify(glm::value_ptr(light_view), sizeof(glm::mat4), sizeof(glm::mat4));
		MeshStatic::get_static_meshes_holder().multi_draw();

		glDepthFunc(GL_ALWAYS);
		depthTexToBuffer.bind();
		matfbo.bind(GL_FRAMEBUFFER);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glDrawArrays(GL_TRIANGLES, 0, 3);

		glDepthFunc(GL_EQUAL);
		materialPipe.bind();
		//glUniform1i(glGetUniformLocation(materialPipe.get_program(), "shadow_map"), 0);
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, shadowfbo.get_depth_stencil_attachment().texture.get_id());

		for (uint32_t i = 0; i < material_data.size(); ++i) {
			material_id.modify(&i,sizeof(uint32_t), 0);
			glDrawArrays(GL_TRIANGLES, 0, 3);
		}

		glDepthFunc(GL_LESS);


		Framebuffer::blit(matfbo, defaultfbo, GL_COLOR_BUFFER_BIT);

		//va.draw(3);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	


	return 0;
}


#endif
