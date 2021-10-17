#include "Graphics_2.h"


GLFWwindow* openWindow() //opens a window and returns it as an object using glfw
{
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		return NULL;
	}
	glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); //want OpenGL 4.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //want modern opengl

	// Open a window and create its OpenGL context
	GLFWwindow* window;
	window = glfwCreateWindow(1920, 1080, "window", NULL, NULL);
	glfwMakeContextCurrent(window); // 

	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible.\n");
		glfwTerminate();
		return window;
	}
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return window;
	}

	return window;
}

void GLAPIENTRY
MessageCallback(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{
	fprintf(stderr, "\nGL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
		(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
		type, severity, message);
}

class InitializeAndOpenWindow
{
public:
	GLFWwindow* window;
	InitializeAndOpenWindow()
	{
		gladLoadGL();
		glfwInit();
		window = openWindow();

#ifndef NDEBUG
		glEnable(GL_DEBUG_OUTPUT);
		glDebugMessageCallback(MessageCallback, 0);
#endif
	}
};

static InitializeAndOpenWindow opengl_and_window_initializer;


GLFWwindow* Graphics::InitiateGraphicsLib(std::vector<std::string> required_extensions){
for (auto itr = required_extensions.begin(); itr < required_extensions.end(); ++itr)
{
	if (glfwExtensionSupported(itr->c_str()) == GLFW_FALSE)
	{
		//todo error
		//renderdoc does not support extensions :(
		std::cout << "ERROR:: Requested Extension: " << *itr << " is not supported\n";
		exit(1);
		break;
	}
}
std::cout << "ALL REQUESTED EXTENSIONS ARE SUPPORTED\n";

return opengl_and_window_initializer.window;
}


void DrawCall()
{}


#ifndef NDEBUG
struct binding_point_info	//size is 12 bytes in release, 16 bytes in debug
{
	Buffer buf;
	unsigned int offset = 0;
	unsigned int size = 0;	//zero means total size
	binding_point_info() = default;
	binding_point_info(Buffer buf, unsigned int offset = 0, unsigned int size = 0) : buf(buf), offset(offset), size(size) {}
};
//I am making this an only debug feature
struct target_bindings_info
{
	std::unordered_map<unsigned int, binding_point_info> uniform_bindings;
	std::unordered_map<unsigned int, binding_point_info> ssbo_bindings;

	std::unordered_map<unsigned int, binding_point_info>& get(GLenum target)
	{
		switch (target)
		{
		case GL_UNIFORM_BUFFER: return uniform_bindings;
			break;
		case GL_SHADER_STORAGE_BUFFER: return ssbo_bindings;
			break;
		default: std::cout << "ERROR INVALID BINDING REQUESTED";
			auto x = (std::unordered_map<unsigned int, binding_point_info> )0;
			return x;
			break;
			
		}
	}
};

static target_bindings_info targetBindingsInformation;

#endif	//in debug config


void Buffer::bind_base(GLenum target, unsigned int binding_point) const
{
	glBindBufferBase(target, binding_point, this->id);

#ifndef NDEBUG
	targetBindingsInformation.get(target)[binding_point] = binding_point_info(*this,0,0);
#endif
}

void Buffer::bind_range(GLenum target, unsigned int binding_point, unsigned int offset, unsigned int size) const
{
	glBindBufferRange(target, binding_point, this->id, offset, size);

#ifndef NDEBUG
	targetBindingsInformation.get(target)[binding_point] = binding_point_info(*this, offset, size);
#endif
}