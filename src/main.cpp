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
#include "Asset/Asset.h"
#include "ECS/ECS.hpp"
#include "Graphics/HigherGraphics_2.h"




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

struct Velocity
{
	Velocity(float val = 0.0f)
	{
		vel = glm::vec3(val);
	}
	glm::vec3 vel;
};

void printVEC3(glm::vec3& vec)
{
	std::cout << '(' << vec.x << ',' << vec.y << ',' << vec.z << ')' << std::endl;
}

struct Position
{
	Position(float val = 1.0f)
	{
		pos = glm::vec3(val);
	}
	glm::vec3 pos;
};
void printPosition(Position& pos_)	{printVEC3(pos_.pos);}




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
	

	
	float vertices_color[] = {
	-0.5f, -0.5f, 0.0f, // left
	 0.0f,1.0f,0.0f,
	 0.5f, -0.5f, 0.0f, // right 
	 0.0f,0.0f,1.0f,
	 0.0f,  0.5f, 0.0f,  // top   
	 1.0f,0.0f,0.0f
	};

	GLFWwindow* window = Graphics::InitiateGraphicsLib({"GL_ARB_bindless_texture"});
	HigherGraphics::initialize();
	
	int window_height, window_width;
	glfwGetWindowSize(window, &window_width, &window_height);
	Texture_Traditional other_tex = Texture_Traditional::create_tex2D_rgb(window_width, window_height);
	Pipeline pipe = Pipeline(R"(C:\Users\munee\source\repos\Node Tree\Node Tree\newDrawVert.glsl)", R"(C:\Users\munee\source\repos\Node Tree\Node Tree\newDrawFrag.glsl)",Framebuffer(window_width,window_height));

	unsigned char x = 10;
	auto tex = Texture_Bindless_2D(1, 1, GL_RED, GL_UNSIGNED_BYTE, &x, GL_RED);
	Texture_Bindless tex2 = tex;
	//3DModelData\\birchTree\\birchTree.obj

	struct
	{
		bool should_load = true;
		const char* what_load = "3DModelData\\birchTree\\birchTree.obj";
		const char* where_store = "";
	}should_load_where_store;

	std::vector<Material*> mats;

	Model model_;

	if (should_load_where_store.should_load)	
	{
		model_.loadModel(should_load_where_store.what_load, false, false);
		mats = model_.get_materials();
		model_.storeModel(should_load_where_store.where_store, "birchTree");
	}

	glm::mat4 projection_matrix = glm::perspective(
			glm::radians(45.0f),	//field of view = 45 degrees
			((float)window_width/ window_height),				//width over height of frustum	//I have set to window width over window height
			0.001f,					//z near
			3000.0f					// z far
		);
	glViewport(0, 0, window_width, window_height);

	Model model2_;
	model2_.loadModel("3DModelData\\StoreTest\\birchTree.twomodel",0,0,0);
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
	pvm_matrices.bind_range(GL_UNIFORM_BUFFER, pipe.get_pipeline_uniform_block_binding("Matrices"), 0, sizeof(glm::mat4) * 3);

	HigherGraphics::get_static_meshes_holder().get_transform_buffer().bind_base(GL_SHADER_STORAGE_BUFFER, 4);

	




	ECSmanager ecs;
	ecs.register_component<MeshStatic*>();
	ecs.register_component<Material*>();

	ecs.register_component<Position>();

	auto meshes_2 = model2_.get_meshes();

	auto meshes_mats = model_.get_meshes_materials();
	for (int i = 0; i < meshes_mats.size(); ++i)
	{
		meshes_mats[i].second->make_resident();
		Position thisOnes(0);
		thisOnes.pos.z = i * 3;
		auto for_static_meshes = ((Mesh3*)meshes_mats[i].first)->debug_get_arrays();
		MeshStatic* new_mesh_test = new MeshStatic(for_static_meshes.first, for_static_meshes.second);
		HigherGraphics::add_instance_of_mesh(new_mesh_test, thisOnes.pos);
		ecs.new_entity(thisOnes, new_mesh_test, meshes_mats[i].second);
	}

	pipe.bind();


	unsigned char pixfnoa[3] = { 255,0,0 };
	Texture_2D sapdpia(1, 1, GL_RGB, GL_UNSIGNED_BYTE, &pixfnoa[0], GL_RGB);
	sapdpia.make_handle_resident();
	int indwasda = 2;
	mats[indwasda]->make_resident();
	//pipe.add_sampler_uniform("albedo");
	//pipe.set_texture_handle("albedo", sapdpia);
	//glUniform1ui64ARB(glGetUniformLocation(pipe.get_program(), "albedo"), sapdpia.get_handle());

	
	Buffer indirect_draw_buff = HigherGraphics::get_static_meshes_holder().create_indirect_draw_buffer();
	indirect_draw_buff.bind(GL_DRAW_INDIRECT_BUFFER);

	ecs.register_system([&cam,&projection_matrix,&pvm_matrices,&pipe](ECSmanager& ecs_manager) 
		{
			auto meshes = ecs_manager.get_components_of_type<MeshStatic*>();
			auto mats  = ecs_manager.get_components_of_type<Material*>();

			auto positions = ecs_manager.get_components_of_type<Position>();
			auto view_matrix = cam.getView();
			pvm_matrices.modify(glm::value_ptr(view_matrix), sizeof(glm::mat4), sizeof(glm::mat4));
			for (int i = 0; i < 1; ++i)
			{
				//pipe.set_texture_handle("albedo", mats[i]->get_handles()[0]);
				HigherGraphics::get_static_meshes_holder().multi_draw();
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


		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		////model2_.draw();
		//model2_.draw();
		ecs.execute_systems();
		//va.draw(3);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	


	return 0;
}