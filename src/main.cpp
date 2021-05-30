#include <iostream>
#include "glm/glm.hpp"
#include "Mesh/Model.h"
int main()
{
	glm::vec3 test(10, 0, 29.5f);
	std::cout << test.x;
	const char* directory = R"(C:\Users\munee\source\repos\Lyra Engine\Lyra Engine\3DModelData\cube\cube.obj)";
	const char* filedir2 = "dooobeedoo";

	Model model;
	model.loadModel(directory,false,true);

	return 1;
}