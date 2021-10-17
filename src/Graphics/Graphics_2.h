#pragma once
#include <assert.h>
#include <iostream>
#include <fstream>
#include "glad/glad.h"
#include <vector>
#include <unordered_map>
#include <string>
#include "GLFW/glfw3.h"
#include <array>
#include <optional>
#include <tuple>


//Maybe wrap all these types of functions under an api class

class Graphics
{
public:
	static GLFWwindow* InitiateGraphicsLib(std::vector<std::string> required_extensions = {});
	
	template<class func_pfn_proc>
	static inline void ext_func_load(func_pfn_proc func_call_name, const char* func_name)
	{
		func_call_name = ((func_pfn_proc)glfwGetProcAddress(func_name));
	}
};






class Buffer
{
private:
	unsigned int id;
#ifndef NDEBUG
	size_t max_size;
#endif // !NDEBUG

public:
	Buffer() = default;
	Buffer(size_t buffer_size, const void* initial_data , GLenum creation_target_buffer = GL_ARRAY_BUFFER, bool make_mutable_storage = false, GLenum mutableBufferUsage = GL_STATIC_DRAW)
	{
#ifndef NDEBUG
		max_size = buffer_size;
#endif
		glCreateBuffers(1, &id);
		glBindBuffer(creation_target_buffer, id);
		if (!make_mutable_storage)
			glBufferStorage(creation_target_buffer, buffer_size, initial_data, GL_DYNAMIC_STORAGE_BIT);
		else
		{
			glBufferData(creation_target_buffer, buffer_size, initial_data, mutableBufferUsage);
#ifndef NDEBUG
			max_size = 0;	//done so that it does not interfere with asserts, since storage size ain't constant
#endif
		}
		//if (creation_target_buffer == GL_ARRAY_BUFFER)
		//{
		//	float* data_got = new float[buffer_size/sizeof(float)];
		//	glGetBufferSubData(creation_target_buffer, 0, buffer_size, data_got);
		//	for (int i = 0; i < buffer_size / sizeof(float); i += 8)
		//	{
		//		//std::cout << '\n'  << data_got[i + 3] << " " << data_got[i+4] << " " << data_got[i+5] << '\n';
		//		if (data_got[i + 3] == 0.0f && data_got[i + 4] == 0.0f && data_got[i + 5] == 0.0f)
		//			std::cout << "bad normal";
		//	}
		//	delete[] data_got;
		//}
	}
	inline void destroy()
	{
		glDeleteBuffers(1, &id);
	}
	inline void modify(const void* data,size_t data_size,size_t offset)
	{
		assert(data_size < max_size);
		glBindBuffer(GL_ARRAY_BUFFER, id);
		glBufferSubData(GL_ARRAY_BUFFER,offset,data_size,data);
	}
	inline void new_data(const void* data, size_t data_size, GLenum usage_type = GL_STATIC_DRAW)
	{
		assert(max_size == 0);
		bind(GL_ARRAY_BUFFER);
		glBufferData(GL_ARRAY_BUFFER, data_size, data, usage_type);
	}
	void copy(size_t size_to_copy, size_t write_buff_offset, const Buffer copy_buff, size_t copy_buff_offset)
	{
		assert(copy_buff.id != id);
		glBindBuffer(GL_COPY_READ_BUFFER, copy_buff.id);
		glBindBuffer(GL_COPY_WRITE_BUFFER, id);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, copy_buff_offset, write_buff_offset, size_to_copy);
	}
	void copy_self(size_t size_to_copy, size_t write_offset, size_t copy_offset)
	{
		//todo test
		glBindBuffer(GL_ARRAY_BUFFER, id);
		glCopyBufferSubData(GL_ARRAY_BUFFER, GL_ARRAY_BUFFER, copy_offset, write_offset, size_to_copy);
	}
	inline void access(void* data_ptr, size_t num_bytes_to_read, size_t offset) const
	{
		glBindBuffer(GL_ARRAY_BUFFER,id);
		glGetBufferSubData(GL_ARRAY_BUFFER, offset, num_bytes_to_read, data_ptr);
	}
	inline void bind(GLenum target) const
	{
		glBindBuffer(target, id);
	}
	void bind_base(GLenum target, unsigned int binding_point) const;	//in source file
	void bind_range(GLenum target, unsigned int binding_point, unsigned int offset, unsigned int size) const;
};
//even though I randomly bind to gl_array_buffer everywhere, this buffer object is to be used exclusively, so if everyone does it its not a problem

struct VertexAttribData
{
	int attrib_size;
	GLenum type;
	GLboolean normalized;
	size_t stride;
	size_t offset;
};

class VertexArray
{
private:
	Buffer v_buffer;
	Buffer i_buffer;
	unsigned int id;
	void vertex_attributes_set(std::vector<VertexAttribData>& attribs)
	{
		v_buffer.bind(GL_ARRAY_BUFFER);
		for (unsigned int i = 0; i < attribs.size(); ++i)
		{
			if (attribs[i].type != GL_FLOAT)
			{
				if (attribs[i].type == GL_UNSIGNED_INT || attribs[i].type == GL_INT || attribs[i].type == GL_SHORT || attribs[i].type == GL_UNSIGNED_SHORT)
					glVertexAttribIPointer(i, attribs[i].attrib_size, attribs[i].type, attribs[i].stride, (void*)attribs[i].offset);
				else
				{
					//this is for double precision floats
					//todo manually normalize if normalized is true
					glVertexAttribLPointer(i, attribs[i].attrib_size, attribs[i].type, attribs[i].stride, (void*)attribs[i].offset);
				}
			}
			else
			{
				glVertexAttribPointer(i, attribs[i].attrib_size, attribs[i].type, attribs[i].normalized, attribs[i].stride, (void*)attribs[i].offset);
			}
			glEnableVertexAttribArray(i);
		}
	}
public:
	VertexArray() = default;
	VertexArray(size_t vertex_data_size, void* vertex_data,size_t num_vertices, std::vector<VertexAttribData> vertex_attributes, GLenum usage = GL_STATIC_DRAW)
	{

		std::vector<unsigned int> index_data(num_vertices);
		for (unsigned int i = 0; i < num_vertices; ++i)
			index_data[i] = i;

		glCreateVertexArrays(1, &id);
		glBindVertexArray(id);

		v_buffer = Buffer(vertex_data_size, vertex_data,GL_ARRAY_BUFFER, true, usage);
		i_buffer = Buffer(index_data.size() * sizeof(unsigned int), &index_data[0],GL_ELEMENT_ARRAY_BUFFER, true, usage);	//TODO test, possible error this

		vertex_attributes_set(vertex_attributes);
	}
	VertexArray(size_t vertex_data_size, void* vertex_data,size_t index_data_size , unsigned int* index_data, std::vector<VertexAttribData> vertex_attributes,GLenum usage = GL_STATIC_DRAW)
	{
		glCreateVertexArrays(1, &id);
		glBindVertexArray(id);

		v_buffer = Buffer(vertex_data_size, vertex_data,GL_ARRAY_BUFFER, true,usage);
		i_buffer = Buffer(index_data_size, index_data,GL_ELEMENT_ARRAY_BUFFER, true, usage);	//TODO test, possible error this
		vertex_attributes_set(vertex_attributes);
	}
	VertexArray(Buffer vertex_buffer, Buffer index_buffer, std::vector<VertexAttribData> vertex_attributes) : v_buffer(vertex_buffer), i_buffer(index_buffer)
	{
		glCreateVertexArrays(1, &id);
		glBindVertexArray(id);

		v_buffer.bind(GL_ARRAY_BUFFER);
		i_buffer.bind(GL_ELEMENT_ARRAY_BUFFER);
		vertex_attributes_set(vertex_attributes);
	}

	void destroy()
	{
		v_buffer.destroy();
		i_buffer.destroy();
		glDeleteVertexArrays(1, &id);
	}
	void draw(unsigned int num_indices, unsigned int indices_offset = 0) const
	{
		glBindVertexArray(id);
		glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, (void*)indices_offset);
	}
	void multi_draw_indirect(unsigned int draw_count, size_t stride = 0, GLsizei indirect_buffer_offset = 0) const
	{
		glBindVertexArray(id);
		glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (const void*)indirect_buffer_offset, draw_count, stride);
	}
	void modify();
	void access() const;
	void bind() const
	{
		glBindVertexArray(id);
	}
	inline unsigned int get_id() const { return id; }
};

class Texture_Traditional
{
private:
	unsigned int id;
	int width, height;	//even for a cubemap this is enough (since its a cube!)		//lmaoo me comments dumb
	int channels;
	GLenum target;


public:
	Texture_Traditional() = default;

	
	static Texture_Traditional create_tex2D(int width_, int height_, GLenum internal_format_, GLenum format_)
	{
		Texture_Traditional tex;
		tex.channels = 3, tex.width = width_, tex.height = height_;
		tex.target = GL_TEXTURE_2D;

		glCreateTextures(GL_TEXTURE_2D, 1, &tex.id);
		glTexImage2D(GL_TEXTURE_2D, 0, internal_format_, width_, height_, 0, format_, GL_UNSIGNED_BYTE, NULL);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		return tex;
	}

	static Texture_Traditional create_tex2D_rgb(int width_, int height_)
	{
		Texture_Traditional tex;
		tex.channels = 3, tex.width = width_, tex.height = height_;
		tex.target = GL_TEXTURE_2D;

		glCreateTextures(GL_TEXTURE_2D,1,&tex.id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width_, height_, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		return tex;
	}

	static Texture_Traditional create_tex2D_rgba(int width_, int height_)
	{
		Texture_Traditional tex;
		tex.channels = 4, tex.width = width_, tex.height = height_;
		tex.target = GL_TEXTURE_2D;

		glCreateTextures(GL_TEXTURE_2D, 1, &tex.id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		return tex;
	}

	static Texture_Traditional create_tex2D_red(int width_, int height_)
	{
		Texture_Traditional tex;
		tex.channels = 1, tex.width = width_, tex.height = height_;
		tex.target = GL_TEXTURE_2D;

		glCreateTextures(GL_TEXTURE_2D, 1, &tex.id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width_, height_, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		return tex;
	}

	/*
	Texture_Traditional(std::string file, GLenum texture_type, unsigned int desired_channels = 0)
	{
		channels = desired_channels;
		target = texture_type;
		glCreateTextures(target, 1, &id);
		int file_channels;
		unsigned char* image_data = stbi_load(file.c_str(), &width, &height, &file_channels, channels);
		if (image_data == NULL)
		{
			std::cout << "error in loading file: " << file << "  For Texture creation";
			exit(1);
		}

		if (target == GL_TEXTURE_2D)
		{
			assert(channels == 3);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
		}
		else if (target == GL_TEXTURE_1D)
		{
			glTexImage1D(GL_TEXTURE_1D, 0, GL_RED, width, 0, GL_RED, GL_UNSIGNED_BYTE, image_data); //TODO test
		}
		else
		{
			std::cout << "invalid target";
			stbi_image_free(image_data);
			exit(1);
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glGenerateMipmap(target);

	}
	*/
	inline void destroy()
	{
		glDeleteTextures(1, &id);
	}
	inline void bind(GLenum target_) const
	{
		glBindTexture(target_, id);
	}
	inline unsigned int get_id() const { return id; }
	void downscale();
	void upscale();
};


class Texture_Parameters
{
private:
	//don't really need integer vector or float vector parameters
	std::vector<std::pair<GLenum,int>> uint_params;
	std::vector<std::pair<GLenum,float>> float_params;
public:
	Texture_Parameters() = default;
	inline void add_param(GLenum parameter_name, int parameter_value) { uint_params.push_back(std::pair(parameter_name, parameter_value)); }
	inline void add_param(GLenum parameter_name, float parameter_value) { float_params.push_back(std::pair(parameter_name, parameter_value)); }

	void parameters_apply_to_currently_bound_texture(GLenum target)
	{
		for (auto itr = uint_params.begin(); itr < uint_params.end(); ++itr)
		{
			glTexParameteri(target, itr->first, itr->second);
		}
		for (auto itr = float_params.begin(); itr < float_params.end(); ++itr)
		{
			glTexParameterf(target, itr->first, itr->second);
		}
	}
};

class Texture_Bindless
{
	//don't care for slicing since there are no virtual functions
protected:
	unsigned int id;
	bool residency;
	uint64_t handle;
	Texture_Bindless() : handle(NULL), residency(false)
	{
		glGenTextures(1, &id);
		//actual handle will be created by class which inherits
	}
public:
	inline void destroy()
	{
		glMakeTextureHandleNonResidentARB(handle);
		glDeleteTextures(1, &id);
	}
	inline void make_handle_resident()
	{
		if (residency) { return; }
		glMakeTextureHandleResidentARB(handle);
		residency = true;
	}
	inline void make_handle_non_resident()
	{
		if (!residency) { return; }
		glMakeTextureHandleNonResidentARB(handle);
		residency = false;
	}
	inline bool is_handle_resident() const { return residency; }
	inline uint64_t get_handle() const { return handle; }

};

class Texture_Bindless_1D : public Texture_Bindless
{
protected:
	uint32_t width;
public:
	Texture_Bindless_1D(uint32_t width, GLenum data_format, GLenum data_type, const void* data, GLenum internal_format) : width(width)
	{
		//Texture_Bindless constructor is called, creating a new texture
		glBindTexture(GL_TEXTURE_1D, id);
		glTexImage1D(GL_TEXTURE_1D, 0, internal_format, width, 0, data_format, data_type, data);
		glGenerateMipmap(GL_TEXTURE_1D);

		handle = glGetTextureHandleARB(id);
		//no state changes after this
	}
};

class Texture_Bindless_2D : public Texture_Bindless
{
protected:
	uint32_t width;
	uint32_t height;
public:
	Texture_Bindless_2D(uint32_t width,uint32_t height, GLenum data_format, GLenum data_type, const void* data, GLenum internal_format) : width(width), height(height)
	{
		//Texture_Bindless constructor is called, creating a new texture
		glBindTexture(GL_TEXTURE_2D, id);
		glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, data_format, data_type, data);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glGenerateMipmap(GL_TEXTURE_2D);


		handle = glGetTextureHandleARB(id);
		//no state changes after this
	}
};

class Texture_Bindless_CubeMap: public Texture_Bindless
{
protected:
	uint32_t width;	//cubemaps only have sidelength
public:
	Texture_Bindless_CubeMap(uint32_t width, GLenum data_format, GLenum data_type,
		const void* data_right, const void* data_left, const void* data_top, const void* data_bottom, const void* data_back, const void* data_front , GLenum internal_format) : width(width)
	{
		//Texture_Bindless constructor is called, creating a new texture
		glBindTexture(GL_TEXTURE_CUBE_MAP, id);

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, internal_format, width,width, 0, data_format, data_type, data_right);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, internal_format, width, width, 0, data_format, data_type, data_left);

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, internal_format, width, width, 0, data_format, data_type, data_top);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, internal_format, width, width, 0, data_format, data_type, data_bottom);

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, internal_format, width, width, 0, data_format, data_type, data_back);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, internal_format, width, width, 0, data_format, data_type, data_front);

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);


		handle = glGetTextureHandleARB(id);
		//no state changes after this
	}

	Texture_Bindless_CubeMap(uint32_t width, GLenum data_format, GLenum data_type, std::array<const void*,6> data, GLenum internal_format) : width(width)
	{
		//Texture_Bindless constructor is called, creating a new texture
		glBindTexture(GL_TEXTURE_CUBE_MAP, id);

		for (int i = 0; i < 6; ++i)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internal_format, width, width, 0, data_format, data_type, data[i]);
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);


		handle = glGetTextureHandleARB(id);
		//no state changes after this
	}
};

class Texture_Bindless_3D : public Texture_Bindless
{
protected:
	uint32_t width;
	uint32_t height;
	uint32_t depth;
public:
	Texture_Bindless_3D(uint32_t width, uint32_t height, uint32_t depth, GLenum data_format, GLenum data_type, const void* data, GLenum internal_format) : width(width), height(height), depth(depth)
	{
		//Texture_Bindless constructor is called, creating a new texture
		glBindTexture(GL_TEXTURE_3D, id);
		glTexImage3D(GL_TEXTURE_3D, 0, internal_format, width, height, depth, 0, data_format, data_type, data);
		glGenerateMipmap(GL_TEXTURE_3D);

		handle = glGetTextureHandleARB(id);
		//no state changes after this
	}
};

typedef Texture_Bindless Texture;
typedef Texture_Bindless_1D Texture_1D;
typedef Texture_Bindless_2D Texture_2D;
typedef Texture_Bindless_CubeMap Texture_CubeMap;
typedef Texture_Bindless_3D Texture_3D;


struct FramebufferAttachmentInfo
{
	bool texture_true_renderbuffer_false = true;
	int width = NULL;
	int height = NULL;
	GLenum internal_format;
};

struct Texture_RenderbufferID
{
public:
	Texture_RenderbufferID() = default;
	bool isTexture;
	union
	{
		Texture_Traditional texture;
		unsigned int renderbuffer_id;
	};
};

class Framebuffer
{
private:
	unsigned int id;
	int width, height;
	std::vector<Texture_Traditional> color_attachments;
	Texture_RenderbufferID depth_stencil_attachment;
	GLenum depth_attach_desc = NULL;
	static unsigned int create_renderbuffer(GLenum internal_format,int width_in, int height_in)
	{
		unsigned int id_renderbuffer;
		glCreateRenderbuffers(1, &id_renderbuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, id_renderbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, internal_format, width_in, height_in);
		return id_renderbuffer;
	}
public:
	Framebuffer(int width_in, int height_in,std::vector<FramebufferAttachmentInfo> attachments)
	{
		width = width_in, height = height_in;
		glCreateFramebuffers(1, &id);
		glBindFramebuffer(GL_FRAMEBUFFER, id);

		//NOW BEHOLD MY GREAT SPAGHETTI DISH
		
		color_attachments.reserve(attachments.size());
		int color_attachment_curr = 0;
		auto attach = attachments.begin();
		for (;attach < attachments.end(); attach++)
		{
			bool isColor = true;
			GLenum format = attach->internal_format;
			if (attach->width == NULL)
				attach->width = width;
			if (attach->height == NULL)
				attach->height = height;
			if (format == GL_DEPTH24_STENCIL8 || format == GL_DEPTH32F_STENCIL8)
			{
				if (depth_attach_desc != NULL)
				{
					//todo error
					exit(1);
				}
				depth_attach_desc = GL_DEPTH_STENCIL_ATTACHMENT;
				format = GL_DEPTH_STENCIL;
				isColor = false;
			}
			else if (format == GL_DEPTH_COMPONENT16 || format == GL_DEPTH_COMPONENT24 || format == GL_DEPTH_COMPONENT32 || format == GL_DEPTH_COMPONENT32F)
			{
				if (depth_attach_desc != NULL)
				{
					//todo error
					exit(1);
				}
				depth_attach_desc = GL_DEPTH_ATTACHMENT;
				format = GL_DEPTH_COMPONENT;
				isColor = false;
			}
			if (attach->texture_true_renderbuffer_false)
			{
				if (format == GL_DEPTH_COMPONENT || format == GL_DEPTH_STENCIL)
				{
					depth_stencil_attachment.isTexture = true;
					depth_stencil_attachment.texture = Texture_Traditional::create_tex2D(attach->width, attach->height, attach->internal_format, format);
					//texture creation leaves it bound to relevent target
					glFramebufferTexture2D(GL_FRAMEBUFFER, depth_attach_desc,GL_TEXTURE_2D,depth_stencil_attachment.texture.get_id(),0);
				}
				else
				{
					color_attachments.push_back(Texture_Traditional::create_tex2D(attach->width, attach->height, attach->internal_format, format));
					//texture creation leaves it bound to relevent target

					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + color_attachment_curr,GL_TEXTURE_2D,color_attachments.back().get_id(),0);
					color_attachment_curr += 1;
				}
			}
			else
			{
				assert(format == GL_DEPTH_COMPONENT || format == GL_DEPTH_STENCIL);
				depth_stencil_attachment.isTexture = false;
				depth_stencil_attachment.renderbuffer_id = Framebuffer::create_renderbuffer(attach->internal_format, attach->width, attach->height);
				//create_renderbuffer leaves it bound
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
					GL_RENDERBUFFER, depth_stencil_attachment.renderbuffer_id);

			}
		}


		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			std::cout << "error in framebuffer creation";
			exit(10);
		}
	}
	Framebuffer(int width_in, int height_in)	:	width(width_in), height(height_in), id(0){}	//default framebuffer
};

class Pipeline
{
private:
	static const unsigned int MAX_BINDING_POINTS_PER_STAGE = 14;
	unsigned int program;
#ifndef  NDEBUG
	std::string shader_file;
#endif // ! NDEBUG


	void link_shader(unsigned int _Shader)
	{
		glAttachShader(program, _Shader);
		glLinkProgram(program);

		//we can now delete the shader since it has been linked to program and now the program has their data in binary
		glDeleteShader(_Shader);
		//ERROR HANDLING FOR LINKING :
		GLint link_ok = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &link_ok);
		//Error handling for linking probs
		if (link_ok == 0)
		{
			GLint maxLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

			// The maxLength includes the NULL character
			std::vector<GLchar> errorLog(maxLength);
			glGetProgramInfoLog(program, maxLength, &maxLength, &errorLog[0]);
			std::cout << "Linking Error" << std::endl << &errorLog[0] << std::endl;
#ifndef NDEBUG
			std::cout << shader_file + "\n";
#endif // !NDEBUG

			// Provide the infolog in whatever manor you deem best.
			// Exit with failure.
			glDeleteProgram(program); // Don't leak the shader
		}
		int infologlen;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infologlen);
		if (infologlen > 0) { std::cout << "ERROR" << std::endl; }
		//return unsigned int program
	}


	struct Buffer_Binding { Buffer buffer;unsigned int binding; Buffer_Binding(Buffer buffer, unsigned int binding) :buffer(buffer), binding(binding) {} };
	//std::array<std::optional<Buffer>, Pipeline::MAX_BINDING_POINTS_PER_STAGE> uniform_buffers;	
	//std::array<std::optional<Buffer>, Pipeline::MAX_BINDING_POINTS_PER_STAGE> ssbo_buffers;
	//reconsidering managing buffers within pipeline, since it is very common to keep the same buffers bound and be used by multiple pipelines

	std::unordered_map<std::string, unsigned int> pipeline_uniform_blocks;
	std::unordered_map<unsigned int, unsigned int> uniform_block_index_to_binding;

	std::unordered_map<std::string, unsigned int> pipeline_ssbo_blocks;
	std::unordered_map<unsigned int, unsigned int> ssbo_block_index_to_binding;

	std::unordered_map<std::string, int> sampler_uniform_locations;

	Framebuffer framebuffer_output;

public:
	static void link_shader_to_program_and_delete_shader(unsigned int _Shader, unsigned int _Program)
	{
		glAttachShader(_Program, _Shader);
		glLinkProgram(_Program);

		//we can now delete the shader since it has been linked to program and now the program has their data in binary
		glDeleteShader(_Shader);
		//ERROR HANDLING FOR LINKING :
		GLint link_ok = 0;
		glGetProgramiv(_Program, GL_LINK_STATUS, &link_ok);
		//Error handling for linking probs
		if (link_ok == 0)
		{
			GLint maxLength = 0;
			glGetProgramiv(_Program, GL_INFO_LOG_LENGTH, &maxLength);

			// The maxLength includes the NULL character
			std::vector<GLchar> errorLog(maxLength);
			glGetProgramInfoLog(_Program, maxLength, &maxLength, &errorLog[0]);
			std::cout << "Linking Error" << std::endl << &errorLog[0] << std::endl;

			// Provide the infolog in whatever manor you deem best.
			// Exit with failure.
			glDeleteProgram(_Program); // Don't leak the shader
		}
		int infologlen;
		glGetProgramiv(_Program, GL_INFO_LOG_LENGTH, &infologlen);
		if (infologlen > 0) { std::cout << "ERROR" << std::endl; }
	}
	static unsigned int compile_shader(unsigned int type, const std::string& source)
	{
		unsigned int _shader;
		const char* src = source.c_str();
		_shader = glCreateShader(type);
		glShaderSource(_shader, 1, &src, nullptr); // READ MORE @ DOCS.GL
		glCompileShader(_shader);

		//ERROR HANDLING(in shader code) BELOW:
		GLint isCompiled = 0;
		glGetShaderiv(_shader, GL_COMPILE_STATUS, &isCompiled);
		if (isCompiled == GL_FALSE)//some error in compiling
		{
			GLint maxLength = 0;
			glGetShaderiv(_shader, GL_INFO_LOG_LENGTH, &maxLength);

			// The maxLength includes the NULL character
			std::vector<GLchar> errorLog(maxLength);
			glGetShaderInfoLog(_shader, maxLength, &maxLength, &errorLog[0]);
			std::cout << "Compiling Error \n"/*+source*/ << std::endl << &errorLog[0] << std::endl;  //prints the error log.. very useful
			// Provide the infolog in whatever manor you deem best.
			// Exit with failure
			glDeleteShader(_shader); // Don't leak the shader.
			unsigned int r = 0;
			return r;
		}
		//return shader
		return _shader;
	}
	static std::string read_shader_file(std::string fileName)
	{

		std::ifstream ifs(fileName);
		if (ifs)
		{
			std::string shaderCode((std::istreambuf_iterator<char>(ifs)),
				(std::istreambuf_iterator<char>()));
			return shaderCode;
		}
		else
		{
			std::cout << "No file found/ file invalid at " << fileName << std::endl;
			return "";
		}
	}
	Pipeline(std::string vertex_shader_loc, std::string fragment_shader_loc,Framebuffer framebuffer) : framebuffer_output(framebuffer)
	{
		program = glCreateProgram();
#ifndef NDEBUG
		shader_file = vertex_shader_loc;
#endif // !NDEBUG

		link_shader(compile_shader(GL_VERTEX_SHADER, read_shader_file(vertex_shader_loc)));							//compiles and links vertex shader to program
		link_shader(compile_shader(GL_FRAGMENT_SHADER, read_shader_file(fragment_shader_loc)));
	}

	inline unsigned int get_program() const
	{
		return program;
	}


	inline void bind() const
	{
		glUseProgram(program);
	}

	inline void add_pipeline_uniform_block(std::string block_name)
	{
		pipeline_uniform_blocks[block_name] = glGetUniformBlockIndex(program, block_name.c_str());
	}

	inline void add_sampler_uniform(std::string uniform_name)
	{
		sampler_uniform_locations[uniform_name] =  glGetUniformLocation(program, uniform_name.c_str());
	}

	inline void bind_pipeline_uniform_block(std::string block_name,unsigned int binding_point)
	{
		unsigned int block_index = pipeline_uniform_blocks[block_name];
		glUniformBlockBinding(program, block_index, binding_point);
		uniform_block_index_to_binding[block_index] = binding_point;
	}

	inline void bind_pipeline_uniform_block(unsigned int block_index,unsigned int binding_point)
	{
		glUniformBlockBinding(program, block_index, binding_point);
		uniform_block_index_to_binding[block_index] = binding_point;
	}

	inline unsigned int get_pipeline_uniform_block_binding(std::string block_name)
	{
		return uniform_block_index_to_binding[pipeline_uniform_blocks[block_name]];
	}

	inline unsigned int get_pipeline_uniform_block_binding(unsigned int block_index)
	{
		return uniform_block_index_to_binding[block_index];
	}

	void add_pipeline_ssbo_block(std::string block_name)
	{
		pipeline_ssbo_blocks[block_name] = glGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, block_name.c_str());
	}

	inline void bind_pipeline_ssbo_block(std::string block_name, unsigned int binding_point)
	{
		unsigned int block_index = pipeline_ssbo_blocks[block_name];
		glShaderStorageBlockBinding(program, block_index, binding_point);
		ssbo_block_index_to_binding[block_index] = binding_point;
	}

	inline unsigned int get_pipeline_ssbo_block_binding(std::string block_name)
	{
		return ssbo_block_index_to_binding[pipeline_ssbo_blocks[block_name]];
	}

	inline unsigned int get_pipeline_ssbo_block_binding(unsigned int block_index)
	{
		return ssbo_block_index_to_binding[block_index];
	}

	inline void set_texture_handle(std::string sampler_uniform_name, Texture_2D& texture)
	{
		assert(texture.is_handle_resident(), "texture handle isn't resident");
		//maybe also cache if pipeline program is active and check
		glUniform1ui64ARB(sampler_uniform_locations[sampler_uniform_name], texture.get_handle());
	}

	inline void set_texture_handle(std::string sampler_uniform_name, uint64_t texture_handle)
	{
#ifndef NDEBUG
		Graphics::ext_func_load(glIsTextureHandleResidentARB, "glIsTextureHandleResidentARB");
		glIsTextureHandleResidentARB(texture_handle);
#endif
		glUniform1ui64ARB(sampler_uniform_locations[sampler_uniform_name], texture_handle);
	}

};

class ComputeShader
{
private:
	unsigned int program;
	std::unordered_map<std::string, unsigned int> ssbo_location;
public:
	ComputeShader(std::string comp_shader_location)
	{
		std::string shader_source = Pipeline::read_shader_file(comp_shader_location);
		program = glCreateProgram();
		unsigned int shader = Pipeline::compile_shader(GL_COMPUTE_SHADER, shader_source);
		Pipeline::link_shader_to_program_and_delete_shader(shader, program);
	}
	void add_ssbo_block(std::string block_name)
	{
		ssbo_location[block_name] = glGetProgramResourceIndex(program,GL_SHADER_STORAGE_BLOCK,block_name.c_str());
	}

	void bind_ssbo_block(std::string block_name, unsigned int binding_point)
	{
		glShaderStorageBlockBinding(program, ssbo_location[block_name], binding_point);
	}

	inline void bind()
	{
		glUseProgram(program);
	}

	inline void dispatch(uint32_t num_groups_x, uint32_t num_groups_y, uint32_t num_groups_z)
	{
		glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
	}
	inline void bind_dispatch(uint32_t num_groups_x, uint32_t num_groups_y, uint32_t num_groups_z)
	{
		glUseProgram(program);
		glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
	}
};




/*
Intended api usage:

Pipeline s;
s.create(xyz);

VertexBuffer y(mesh0.get_data());

s.set_active();

y.draw_call();

*/