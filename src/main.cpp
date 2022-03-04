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

	return std::make_shared<MeshStatic>(vertices, indices);
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
		R"(C:\Users\munee\source\repos\Engine-2\src\Graphics\newDrawVert.glsl)",
		R"(C:\Users\munee\source\repos\Engine-2\src\Graphics\newDrawFrag.glsl)");

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
	std::vector<glm::vec4> material_data;

	for (const auto& file : std::filesystem::directory_iterator("C:\\Users\\munee\\source\\repos\\Engine-2\\3DModelData\\birchTwoMesh")) {
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
	Texture_2D sapdpia(1, 1, GL_RGB, GL_UNSIGNED_BYTE, &pixfnoa[0], GL_RGB);
	sapdpia.make_handle_resident();
	int indwasda = 2;
	//pipe.add_sampler_uniform("albedo");
	//pipe.set_texture_handle("albedo", sapdpia);
	//glUniform1ui64ARB(glGetUniformLocation(pipe.get_program(), "albedo"), sapdpia.get_handle());

	MeshStatic::prepare_indirect_draw_buffer();
	pipe.bind();
	const Buffer& indirect_draw_buff = MeshStatic::get_static_meshes_holder().get_indirect_draw_buff();
	indirect_draw_buff.bind(GL_DRAW_INDIRECT_BUFFER);

	Framebuffer otherfbo(window_width, window_height,
		{ FramebufferAttachment(Texture_2D(window_width,window_height,GL_RGB32F),true),
		  FramebufferAttachment(Texture_2D(window_width,window_height,GL_RGB32F),true),
		  FramebufferAttachment(Texture_2D(window_width,window_height,GL_RED,GL_UNSIGNED_INT,NULL, GL_R32F),true) },
		FramebufferAttachment(Renderbuffer(window_width, window_height, GL_DEPTH24_STENCIL8), false),
		DepthStencilAttachType::BOTH_DEPTH_STENCIL);
	otherfbo.bind(GL_FRAMEBUFFER);
	otherfbo.set_read_buffer(0);
	for (auto& _attachment : otherfbo.get_color_attachments()) {
		_attachment.texture.make_handle_resident();
	}
	uint64_t gbuffer_handles[2];
	for (int i = 0; i < 2; ++i) {
		gbuffer_handles[i] = otherfbo.get_color_attachments()[i].texture.get_handle();
	}

	Framebuffer matfbo(window_width, window_height,
		{ FramebufferAttachment(Renderbuffer(window_width,window_height,GL_RGBA),true) },
		FramebufferAttachment(Renderbuffer(window_width, window_height, GL_DEPTH_COMPONENT32F), false),
		DepthStencilAttachType::ONLY_DEPTH);

	Pipeline depthTexToBuffer(R"(src\Graphics\FullScreenVert.glsl)", R"(src\Graphics\TextureToDepthFrag.glsl)");

	glProgramUniformHandleui64ARB(depthTexToBuffer.get_program(),
		glGetAttribLocation(depthTexToBuffer.get_program(), "depth_texture"),
		otherfbo.get_color_attachments()[2].texture.get_handle());


	Pipeline materialPipe(R"(src\Graphics\MaterialShaderVert.glsl)", R"(src\Graphics\MaterialShader.glsl)");
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

		glDepthFunc(GL_ALWAYS);
		depthTexToBuffer.bind();
		matfbo.bind(GL_FRAMEBUFFER);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glDrawArrays(GL_TRIANGLES, 0, 3);

		glDepthFunc(GL_EQUAL);
		materialPipe.bind();
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
