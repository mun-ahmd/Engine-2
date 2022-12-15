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
#include <numeric>
#include <cstring>


class Graphics
{
public:
	static GLFWwindow* InitiateGraphicsLib(std::vector<std::string> required_extensions = {});
	
	template<class func_pfn_proc>
	static inline void ext_func_load(func_pfn_proc func_call_name, const char* func_name)
	{
		func_call_name = ((func_pfn_proc)glfwGetProcAddress(func_name));
	}

	static inline void clear_color(float r, float g, float b, float a) {
		glClearColor(r, g, b, a);
	}

	static std::string error_check() {
		GLenum err;
		while ((err = glGetError()) != GL_NO_ERROR)
		{
			switch (err) {
				case GL_INVALID_ENUM:
					return "GL_INVALID_ENUM";
				case GL_INVALID_OPERATION:
					return "GL_INVALID_OPERATION";
				case GL_INVALID_VALUE:
					return "GL_INVALID_VALUE";
				case GL_INVALID_FRAMEBUFFER_OPERATION:
					return "GL_INVALID_FRAMEBUFFER_OPERATION";
				case GL_OUT_OF_MEMORY:
					return "GL_OUT_OF_MEMORY";
				case GL_STACK_UNDERFLOW:
					return "GL_STACK_UNDERFLOW";
				case GL_STACK_OVERFLOW:
					return "GL_STACK_OVERFLOW";
				default:
					return "UNKNOWN ERROR CODE: " + std::to_string(static_cast<unsigned int>(err));
			}
		}
		return "NO ERROR";
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
	}
	inline void destroy()
	{
		glDeleteBuffers(1, &id);
	}
	inline void modify(const void* data,size_t data_size,size_t offset)
	{
		//assert(data_size < max_size);
		glNamedBufferSubData(id, offset, data_size, data);
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

struct VertexAttribData
{
	int attrib_size;
	GLenum type;
	GLboolean normalized;
	size_t stride;
	size_t offset;
	VertexAttribData() = default;
	VertexAttribData(int attrib_size, GLenum type, GLboolean normalized, size_t stride, size_t offset)
		: attrib_size(attrib_size), type(type), normalized(normalized), stride(stride), offset(offset)
	{}
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
	VertexArray(size_t vertex_data_size, const void* vertex_data,size_t num_vertices, std::vector<VertexAttribData> vertex_attributes, GLenum usage = GL_STATIC_DRAW)
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
	VertexArray(size_t vertex_data_size, const void* vertex_data,size_t index_data_size , const unsigned int* index_data, std::vector<VertexAttribData> vertex_attributes,GLenum usage = GL_STATIC_DRAW)
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

		this->v_buffer.bind(GL_ARRAY_BUFFER);
		this->i_buffer.bind(GL_ELEMENT_ARRAY_BUFFER);
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
		glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, (const void*)indices_offset);
	}
	void draw_arrays(unsigned int first, unsigned int count) {
		glBindVertexArray(id);
		glDrawArrays(GL_TRIANGLES, first, count);
	}
	void multi_draw_indirect(unsigned int draw_count, size_t stride = 0, unsigned int indirect_buffer_offset = 0) const
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

template<typename T>
struct TypeToGLtype { static constexpr GLenum GLtype = GL_NONE; };
template<> struct TypeToGLtype<float> { static constexpr GLenum GLtype = GL_FLOAT; };
template<> struct TypeToGLtype<double> { static constexpr GLenum GLtype = GL_DOUBLE; };
template<> struct TypeToGLtype<int> { static constexpr GLenum GLtype = GL_INT; };
template<> struct TypeToGLtype<unsigned int> { static constexpr GLenum GLtype = GL_UNSIGNED_INT; };
template<> struct TypeToGLtype<short> { static constexpr GLenum GLtype = GL_SHORT; };
template<> struct TypeToGLtype<unsigned short> { static constexpr GLenum GLtype = GL_UNSIGNED_SHORT; };
template<> struct TypeToGLtype<char> { static constexpr GLenum GLtype = GL_BYTE; };
template<> struct TypeToGLtype<unsigned char> { static constexpr GLenum GLtype = GL_UNSIGNED_BYTE; };

class Texture_Parameters
{
private:
	//don't really need integer vector or float vector parameters
	std::vector<std::pair<GLenum,int>> uint_params;
	std::vector<std::pair<GLenum,float>> float_params;
public:
	Texture_Parameters() = default;
	inline void add_param(GLenum parameter_name, int parameter_value) { uint_params.push_back(std::make_pair(parameter_name, parameter_value)); }
	inline void add_param(GLenum parameter_name, float parameter_value) { float_params.push_back(std::make_pair(parameter_name, parameter_value)); }

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


class Sampler {
private:
	unsigned int id;
	//bool immutability = false;	//once associated with handle it cannot be changed
	Sampler(unsigned int id) : id(id) {	/*only used to create default sampler*/ }
public:
	static Sampler default_sampler() {
		return Sampler(0);
	}
	Sampler() {
		glCreateSamplers(1, &id);
	}
	inline unsigned int get_id() {
		return id;
	}
	inline void bind_to_unit(unsigned int unit) {
		glBindSampler(unit, this->get_id());
	}
	inline void change_parameter(GLenum param_name, int param_val) {
		glSamplerParameteri(id, param_name, param_val);
	}
	inline void change_parameter(GLenum param_name, float param_val) {
		glSamplerParameterf(id, param_name, param_val);
	}
	inline void change_parameterv(GLenum param_name, int* params) {
		glSamplerParameteriv(id, param_name, params);
	}
	inline void change_parameterv(GLenum param_name, float* params) {
		glSamplerParameterfv(id, param_name, params);
	}
	inline void destroy() {
		glDeleteSamplers(1, &id);
	}
};

class Texture_Bindless
{
	//don't care for slicing since there are no virtual functions
private:
protected:
	Texture_Bindless() { /*only exists for default framebuffer hacky saw */ };

	unsigned int id;
	bool residency;
	uint64_t handle;
	Sampler active_sampler = Sampler::default_sampler();

	Texture_Bindless(GLenum target) : residency(false), handle(NULL)
	{
		glCreateTextures(target, 1, &id);
		glBindTexture(target, id);
		//this will also bind the texture (to minimize number of calls)
		//actual handle will be created by class which inherits
	}
	void change_handle(uint64_t new_handle) {
		if (residency) {
			//todo this would make it so
			//if there was a bindless texture set in a shader program
			//it would no longer work since the handle there is invalid
			glMakeTextureHandleNonResidentARB(this->handle);
			glMakeTextureHandleResidentARB(new_handle);
		}
		this->handle = new_handle;
	}
	constexpr GLenum true_internal_format(GLenum internal_format_in) {
		switch (internal_format_in)
		{
		case GL_DEPTH_COMPONENT:
			return GL_DEPTH_COMPONENT32;
		case GL_RED:
			return GL_R8;
		case GL_RG:
			return GL_RG8;
		case GL_RGB:
			return GL_RGB8;
		case GL_RGBA:
			return GL_RGBA8;
		default:
			return internal_format_in;
			break;
		}
	}

public:
	inline void destroy()
	{
		if(residency)
			glMakeTextureHandleNonResidentARB(handle);
		glDeleteTextures(1, &id);
	}
	inline unsigned int get_id() const { return id; }

	void change_sampler(Sampler& sampler) {
		change_handle(glGetTextureSamplerHandleARB(id, sampler.get_id()));
		this->active_sampler = sampler;
	}
	
	void remove_sampler() {
		change_handle(glGetTextureHandleARB(id));
		this->active_sampler = Sampler::default_sampler();
	}

	inline void change_parameter(GLenum param_name, int param_val) {
		// 
		// todo this won't work since texture handle creation makes it impossible to modify texture params
		// to fix create a Sampler class that stores texture sampling parameters
		// and you can change sampler of this texture by creating a new handler after making the last one non resident
		// using glGetTextureSamplerHandleARB
		// SAMPLERS DONE
		// 
		// create a function that returns a new texture which is a view of this tex
		// and create a different view when you need different params
		// PENDING
		// 
		// also make it easy to set params on construction

		glTextureParameteri(this->id, param_name, param_val);
	}

	//bindless texture methods:
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

	//traditional texture methods:
	inline void bind_to_unit(GLenum target, unsigned int texture_unit) {
		glBindTextureUnit(texture_unit, this->id);
		if(this->active_sampler.get_id() != 0)
			this->active_sampler.bind_to_unit(texture_unit);
		//todo keep track of what texture is bound to each unit
	}


	void bind_image_texture(unsigned int unit, GLenum access, GLenum format, bool layered, unsigned int level = 0, unsigned int layer = 0) {
		//layered should be true in case of 3D texture
		glBindImageTexture(unit, id, level, layered, layer, access, format);
	}

	static Texture_Bindless __default_framebuffer_texture__() {
		Texture_Bindless t;
		t.id = 0;
		t.residency = false;
		return t;
	}
};

class Texture_Bindless_1D : public Texture_Bindless
{
protected:
	uint16_t width;
public:
	Texture_Bindless_1D(uint16_t width, GLenum internal_format, GLenum data_format, GLenum data_type, const void* data) : Texture_Bindless(GL_TEXTURE_1D), width(width)
	{
		//Texture_Bindless constructor is called, creating a new texture, binding it
		glTexImage1D(GL_TEXTURE_1D, 0, internal_format, width, 0, data_format, data_type, data);
		glGenerateMipmap(GL_TEXTURE_1D);

		handle = glGetTextureHandleARB(id);
		//no state changes after this
	}
};

class Texture_Bindless_2D : public Texture_Bindless
{
protected:
	uint16_t width;
	uint16_t height;

	Texture_Bindless_2D() : Texture_Bindless(Texture_Bindless::__default_framebuffer_texture__()), width(100), height(100) {
		//todo not a great piece of code, but saves time for creating default framebuffer
	}


public:
	Texture_Bindless_2D(uint16_t width, uint16_t height, GLenum internal_format)
		: Texture_Bindless(GL_TEXTURE_2D), width(width), height(height){
		//Texture_Bindless constructor is called, creating a new texture, binding it
		glTextureStorage2D(id, 1, true_internal_format(internal_format), width, height);
		glGenerateTextureMipmap(id);

		glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_REPEAT);

		handle = glGetTextureHandleARB(id);
		//no state changes after this
	}

	Texture_Bindless_2D(uint16_t width, uint16_t height, GLenum internal_format,
		GLenum data_format, GLenum data_type, const void* data) 
		: Texture_Bindless_2D(width, height, internal_format){
		//only subbing tex data is required
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, data_format, data_type, data);
	}

	template<typename T>
	void sub_texture_data(int x_off, int y_off, int width, int height, GLenum data_format, const T* data) {
		glBindTexture(GL_TEXTURE_2D, this->id);
		glTexSubImage2D(GL_TEXTURE_2D, 0, x_off, y_off, width, height, data_format, TypeToGLtype<T>::GLtype, data);
	}

	inline uint16_t get_width() const { return width; }
	inline uint16_t get_height() const { return height; }

	static Texture_Bindless_2D __default_framebuffer_texture2D__() {
		return Texture_Bindless_2D();
	}
};

class Texture_Bindless_CubeMap: public Texture_Bindless
{
protected:
	uint16_t width;	//cubemaps only have sidelength
public:
	Texture_Bindless_CubeMap(uint16_t width, GLenum internal_format, GLenum data_format, GLenum data_type,
		const void* data_right, const void* data_left, const void* data_top, const void* data_bottom, const void* data_back, const void* data_front) : Texture_Bindless(GL_TEXTURE_CUBE_MAP) ,width(width)
	{
		//Texture_Bindless constructor is called, creating a new texture and binding it

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

	Texture_Bindless_CubeMap(uint16_t width, GLenum internal_format, GLenum data_format, GLenum data_type, std::array<const void*,6> data)
		: Texture_Bindless(GL_TEXTURE_CUBE_MAP) ,width(width)
	{
		//Texture_Bindless constructor is called, creating a new texture, binding it

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
	uint16_t width;
	uint16_t height;
	uint16_t depth;
public:
	Texture_Bindless_3D() = default;

	Texture_Bindless_3D(uint16_t width, uint16_t height, uint16_t depth, GLenum internal_format)
		: Texture_Bindless(GL_TEXTURE_3D), width(width), height(height), depth(depth)
	{
		//Texture_Bindless constructor is called, creating a new texture and binding it
		glTextureStorage3D(this->id, 1, true_internal_format(internal_format), width, height, depth);
		glGenerateTextureMipmap(id);

		glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_REPEAT);

		handle = glGetTextureHandleARB(id);
		//no state changes after this
	}

	Texture_Bindless_3D(uint16_t width, uint16_t height, uint16_t depth, GLenum internal_format, GLenum data_format, GLenum data_type, const void* data)
		: Texture_Bindless_3D(width, height, depth, internal_format) {
		glTextureSubImage3D(id, 0, 0, 0, 0, width, height, depth, data_format, data_type, data);
	}
};

class Texture_Bindless_2D_Array : public Texture_Bindless {
protected:
	uint16_t width;
	uint16_t height;
	uint16_t num_layers;
public:
	Texture_Bindless_2D_Array() = default;

	Texture_Bindless_2D_Array(uint16_t width, uint16_t height, uint16_t num_layers, GLenum internal_format)
		: Texture_Bindless(GL_TEXTURE_2D_ARRAY), width(width), height(height), num_layers(num_layers)
	{
		//Texture_Bindless constructor is called, creating a new texture and binding it
		glBindTexture(GL_TEXTURE_2D_ARRAY, id);
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, true_internal_format(internal_format), width, height, num_layers);
		//glGenerateMipmap(GL_TEXTURE_3D);

		glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_REPEAT);

		handle = glGetTextureHandleARB(id);
		//no state changes after this
	}

	Texture_Bindless_2D_Array(uint16_t width, uint16_t height, uint16_t num_layers, GLenum internal_format, GLenum data_format, GLenum data_type, const void* data)
		: Texture_Bindless_2D_Array(width, height, num_layers, internal_format) {
		glTextureSubImage3D(id, 0, 0, 0, 0, width, height, num_layers, data_format, data_type, data);
	}
};

typedef Texture_Bindless Texture;
typedef Texture_Bindless_1D Texture_1D;
typedef Texture_Bindless_2D Texture_2D;
typedef Texture_Bindless_CubeMap Texture_CubeMap;
typedef Texture_Bindless_3D Texture_3D;
typedef Texture_Bindless_2D_Array Texture_2D_Array;


class Renderbuffer {
private:
	unsigned int id;
	uint16_t width;
	uint16_t height;
public:
	Renderbuffer(uint16_t width, uint16_t height, GLenum internal_format) : width(width), height(height) {
		glCreateRenderbuffers(1, &id);
		glNamedRenderbufferStorage(id, internal_format, width, height);
	}

	inline void destroy() {
		glDeleteRenderbuffers(1, &id);
	}

	inline unsigned int get_id() const { return id; }
	inline uint16_t get_width() const { return width; }
	inline uint16_t get_height() const { return height; }

};


struct FramebufferAttachment
{
private:
	FramebufferAttachment(Texture texture, bool is_draw_buffer, int texture_dims, int z_offset = -1)
		: is_draw_buffer(is_draw_buffer), is_texture(true), texture_dims(texture_dims), z_offset(z_offset), texture(texture) {}
public:
	bool is_draw_buffer;
	bool is_texture;
	int texture_dims;
	int z_offset;
	union {
		Texture texture;
		Renderbuffer renderbuffer;
	};

	static FramebufferAttachment new_layered_tex(Texture texture, bool is_draw_buffer) {
		return FramebufferAttachment(texture, is_draw_buffer, 0);
	}
	FramebufferAttachment() {
	};
	FramebufferAttachment(Texture_1D texture, bool is_draw_buffer)
		: FramebufferAttachment(texture, is_draw_buffer, 1) {}
	FramebufferAttachment(Texture_2D texture, bool is_draw_buffer)
		: FramebufferAttachment(texture, is_draw_buffer, 2) {}
	FramebufferAttachment(Texture_3D texture, bool is_draw_buffer, int z_offset)
		: FramebufferAttachment(texture, is_draw_buffer, 3, z_offset) {}
	FramebufferAttachment(Renderbuffer renderbuffer, bool is_draw_buffer)
		: is_draw_buffer(is_draw_buffer), is_texture(false), texture_dims(-1), z_offset(-1), renderbuffer(renderbuffer) {}

	//uint16_t get_width() {
	//	if (is_texture)
	//		return texture.get_width();
	//	else
	//		return renderbuffer.get_width();
	//}
	//uint16_t get_height() {
	//	if (is_texture)
	//		return texture.get_height();
	//	else
	//		return renderbuffer.get_height();
	//}
};

enum class DepthStencilAttachType {
	ONLY_DEPTH = GL_DEPTH_ATTACHMENT,
	ONLY_STENCIL = GL_STENCIL_ATTACHMENT,
	BOTH_DEPTH_STENCIL = GL_DEPTH_STENCIL_ATTACHMENT,
	DEFAULT_FRAMEBUFFER
};

class Framebuffer
{
private:
	unsigned int id;
	uint16_t width, height;
	std::vector<FramebufferAttachment> color_attachments;

	DepthStencilAttachType depth_stencil_attach_type;
	FramebufferAttachment depth_stencil_attachment;

	Framebuffer(uint16_t width, uint16_t height) : width(width), height(height),
		id(0), depth_stencil_attachment(FramebufferAttachment(Texture_Bindless_2D::__default_framebuffer_texture2D__(),1)),
		depth_stencil_attach_type(DepthStencilAttachType::DEFAULT_FRAMEBUFFER) {
		//todo again not a great piece of code (pretty bad actually), but saves dev time
	}
public:
	Framebuffer() = default;

	Framebuffer(uint16_t width, uint16_t height, 
		std::vector<FramebufferAttachment> color_attachments,
		FramebufferAttachment depth_stencil_attachment,
		DepthStencilAttachType depth_stencil_attach_type = DepthStencilAttachType::BOTH_DEPTH_STENCIL
	)
		: width(width), height(height),
		color_attachments(color_attachments),
		depth_stencil_attachment(depth_stencil_attachment),
		depth_stencil_attach_type(depth_stencil_attach_type)
	{
		glCreateFramebuffers(1, &id);
		glBindFramebuffer(GL_FRAMEBUFFER, id);

		unsigned int color_attachment_i = 0;
		std::vector<GLenum> draw_color_attachments;
		draw_color_attachments.reserve(this->color_attachments.size());
		for (const auto& attach : this->color_attachments) {
			if (attach.is_draw_buffer)
				draw_color_attachments.push_back(GL_COLOR_ATTACHMENT0 + color_attachment_i);

			if (attach.is_texture)
				switch (attach.texture_dims) {
				case 0:
					glNamedFramebufferTexture(id, GL_COLOR_ATTACHMENT0 + color_attachment_i, attach.texture.get_id(), 0);
					break;
				case 1:
					glFramebufferTexture1D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + color_attachment_i, GL_TEXTURE_1D, attach.texture.get_id(), 0);
					break;
				case 2:
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + color_attachment_i, GL_TEXTURE_2D, attach.texture.get_id(), 0);
					break;
				case 3:
					glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + color_attachment_i, GL_TEXTURE_3D, attach.texture.get_id(), 0, attach.z_offset);
					break;
				}
			else
				glNamedFramebufferRenderbuffer(id, GL_COLOR_ATTACHMENT0 + color_attachment_i, GL_RENDERBUFFER, attach.renderbuffer.get_id());
			
			color_attachment_i++;
		}
		if (draw_color_attachments.empty())
			glNamedFramebufferDrawBuffer(id, GL_NONE);
		else
			glNamedFramebufferDrawBuffers(id, draw_color_attachments.size(), draw_color_attachments.data());

		if (depth_stencil_attachment.is_texture)
			glNamedFramebufferTexture(id, (GLenum)depth_stencil_attach_type, depth_stencil_attachment.texture.get_id(), 0);
		else
			glNamedFramebufferRenderbuffer(id, (GLenum)depth_stencil_attach_type, GL_RENDERBUFFER, depth_stencil_attachment.renderbuffer.get_id());

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			//todo handle
			std::cout << "error in framebuffer creation";
			exit(10);
		}
	}

	static Framebuffer default_framebuffer(uint32_t width, uint32_t height) {
		return Framebuffer(width, height);
	}

	inline void bind(GLenum target) {
		glBindFramebuffer(target, id);
	}

	inline void clear(GLenum mask) {
		//TODO REMOVE LATER
		bind(GL_FRAMEBUFFER);
		glClear(mask);
	}
	
	inline uint16_t get_width() const { return width; }
	inline uint16_t get_height() const { return height; }
	inline unsigned int get_id() const { return id; }
	inline auto& get_color_attachments() { return this->color_attachments; }
	inline auto& get_depth_stencil_attachment() { return this->depth_stencil_attachment; }

	inline void set_read_buffer(unsigned int color_attachment_index) {
		glNamedFramebufferReadBuffer(id, GL_COLOR_ATTACHMENT0 + color_attachment_index);
	}

	static void blit(const Framebuffer& source, Framebuffer& dest, GLenum mask, GLenum filter = GL_LINEAR) {
		auto X1 = std::min(source.width, dest.width);
		auto Y1 = std::min(source.height, dest.height);
		glBlitNamedFramebuffer(source.id, dest.id, 0, 0, X1, Y1, 0, 0, X1, Y1, mask, filter);
	}

	static void blit(const Framebuffer& source, Framebuffer& dest, 
		uint16_t src_x_0, uint16_t src_x_1, uint16_t src_y_0, uint16_t src_y_1,
		uint16_t dest_x_0, uint16_t dest_x_1, uint16_t dest_y_0, uint16_t dest_y_1,
		GLenum mask, GLenum filter = GL_LINEAR)
	{
		//todo write assert to check bounds
		glBlitNamedFramebuffer(
			source.id, dest.id,
			src_x_0, src_y_0, src_x_1, src_y_1,
			dest_x_0, dest_y_0, dest_x_1, dest_y_1,
			mask, filter);
	}
};



class Shader {
private:
	unsigned int shader;

	std::pair<bool, std::string> check_error() {
		GLint isCompiled = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
		if (isCompiled == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

			// The maxLength includes the NULL character
			std::vector<GLchar> errorLog(maxLength);
			glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);
			return std::make_pair(true, std::string(errorLog.data()));
		}
		return std::make_pair(false, std::string(""));
	}
public:
	//todo tesselation shader support
	enum class Type {
		VERTEX = GL_VERTEX_SHADER,
		FRAGMENT = GL_FRAGMENT_SHADER,
		GEOMETRY = GL_GEOMETRY_SHADER,
		COMPUTE = GL_COMPUTE_SHADER
	};
	Shader(std::string source_code, Shader::Type type) {
		const char* src = source_code.c_str();
		shader = glCreateShader(static_cast<GLenum>(type));
		glShaderSource(shader, 1, &src, nullptr); // READ MORE @ DOCS.GL
		glCompileShader(shader);

		auto error_check = check_error();
		if (error_check.first == true) {
			//todo ERROR
			std::cout << error_check.second << std::endl;	//print error log
			exit(1001);
		}
	}

	inline unsigned int get_shader_id() {
		return shader;
	}
};


class Pipeline
{
private:
	static const unsigned int MAX_BINDING_POINTS_PER_STAGE = 14;

	using UniformFloatMatrixFunc = void(*)(GLuint, GLint, GLsizei, GLboolean, const GLfloat*);
	inline static const std::array<UniformFloatMatrixFunc, 16> uniform_float_mat_funcs = {
		nullptr, nullptr, nullptr, nullptr,
		nullptr, glProgramUniformMatrix2fv, glProgramUniformMatrix3x2fv, glProgramUniformMatrix4x2fv,
		nullptr, glProgramUniformMatrix2x3fv, glProgramUniformMatrix3fv, glProgramUniformMatrix4x3fv,
		nullptr, glProgramUniformMatrix2x4fv, glProgramUniformMatrix3x4fv, glProgramUniformMatrix4fv
	};
	using UniformDoubleMatrixFunc = void(*)(GLuint, GLint, GLsizei, GLboolean, const double*);
	inline static const std::array<UniformDoubleMatrixFunc, 16> uniform_double_mat_funcs = {
	nullptr, nullptr, nullptr, nullptr,
	nullptr, glProgramUniformMatrix2dv, glProgramUniformMatrix3x2dv, glProgramUniformMatrix4x2dv,
	nullptr, glProgramUniformMatrix2x3dv, glProgramUniformMatrix3dv, glProgramUniformMatrix4x3dv,
	nullptr, glProgramUniformMatrix2x4dv, glProgramUniformMatrix3x4dv, glProgramUniformMatrix4dv
	};

	unsigned int program;

#ifndef  NDEBUG
	std::string shader_file;
#endif // ! NDEBUG


	struct Buffer_Binding { Buffer buffer;unsigned int binding; Buffer_Binding(Buffer buffer, unsigned int binding) :buffer(buffer), binding(binding) {} };
	//std::array<std::optional<Buffer>, Pipeline::MAX_BINDING_POINTS_PER_STAGE> uniform_buffers;	
	//std::array<std::optional<Buffer>, Pipeline::MAX_BINDING_POINTS_PER_STAGE> ssbo_buffers;
	//reconsidering managing buffers within pipeline, since it is very common to keep the same buffers bound and be used by multiple pipelines

	std::unordered_map<std::string, unsigned int> pipeline_uniform_blocks;
	std::unordered_map<unsigned int, unsigned int> uniform_block_index_to_binding;

	std::unordered_map<std::string, unsigned int> pipeline_ssbo_blocks;
	std::unordered_map<unsigned int, unsigned int> ssbo_block_index_to_binding;

	std::unordered_map<std::string, int> sampler_uniform_locations;

	GLenum depth_func = GL_LESS;

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
			glGetProgramInfoLog(program, maxLength, &maxLength, errorLog.data());
			if (errorLog.size() == 0)
				std::cout << "Linking Error" << std::endl << "No Error Log";
			else
				std::cout << "Linking Error" << std::endl << errorLog.data() << std::endl;

			// Provide the infolog in whatever manor you deem best.
			// Exit with failure.
			glDeleteProgram(program); // Don't leak the shader
		}
		else {
			GLint maxLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

			// The maxLength includes the NULL character
			std::vector<GLchar> errorLog(maxLength);
			glGetProgramInfoLog(program, maxLength, &maxLength, errorLog.data());
			if (errorLog.size() != 0)
				std::cout << "Info Log: " << std::endl << errorLog.data() << std::endl;
		}
	}



public:
	Pipeline() {}

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
			glGetProgramInfoLog(_Program, maxLength, &maxLength, errorLog.data());
			if (errorLog.size() == 0)
				std::cout << "Linking Error" << std::endl << "No Error Log";
			else
				std::cout << "Linking Error" << std::endl << errorLog.data() << std::endl;

			// Provide the infolog in whatever manor you deem best.
			// Exit with failure.
			glDeleteProgram(_Program); // Don't leak the shader
		}
		else {
			GLint maxLength = 0;
			glGetProgramiv(_Program, GL_INFO_LOG_LENGTH, &maxLength);

			// The maxLength includes the NULL character
			std::vector<GLchar> errorLog(maxLength);
			glGetProgramInfoLog(_Program, maxLength, &maxLength, errorLog.data());
			if (errorLog.size() != 0)
				std::cout << "Info Log: " << std::endl << errorLog.data() << std::endl;
		}
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
	Pipeline(std::string vertex_shader_loc, std::string fragment_shader_loc)
	{
		program = glCreateProgram();
#ifndef NDEBUG
		shader_file = vertex_shader_loc;
#endif // !NDEBUG

		link_shader(compile_shader(GL_VERTEX_SHADER, read_shader_file(vertex_shader_loc)));							//compiles and links vertex shader to program
		link_shader(compile_shader(GL_FRAGMENT_SHADER, read_shader_file(fragment_shader_loc)));
	}
	static Pipeline create_from_source(std::string vertex_shader_src, std::string frag_shader_src) {
		Pipeline pipe;
		pipe.program = glCreateProgram();
		pipe.link_shader(compile_shader(GL_VERTEX_SHADER, vertex_shader_src));
		pipe.link_shader(compile_shader(GL_FRAGMENT_SHADER, frag_shader_src));
		return pipe;
	}
	static Pipeline new_vert_frag(std::string vertex_shader_src, std::string frag_shader_src) {
		Pipeline pipe;
		pipe.program = glCreateProgram();
		pipe.link_shader(compile_shader(GL_VERTEX_SHADER, vertex_shader_src));
		pipe.link_shader(compile_shader(GL_FRAGMENT_SHADER, frag_shader_src));
		return pipe;
	}
	static Pipeline new_vert_geometry_frag(std::string vertex_shader_src, std::string geometry_shader_src, std::string frag_shader_src) {
		Pipeline pipe;
		pipe.program = glCreateProgram();
		pipe.link_shader(compile_shader(GL_VERTEX_SHADER, vertex_shader_src));
		pipe.link_shader(compile_shader(GL_GEOMETRY_SHADER, geometry_shader_src));
		pipe.link_shader(compile_shader(GL_FRAGMENT_SHADER, frag_shader_src));
		return pipe;
	}

	inline unsigned int get_program() const
	{
		return program;
	}

	inline void destroy() {
		glDeleteProgram(program);
		program = 0;
		pipeline_ssbo_blocks.clear();
		ssbo_block_index_to_binding.clear();
		pipeline_uniform_blocks.clear();
		uniform_block_index_to_binding.clear();
		sampler_uniform_locations.clear();
	}

	inline void bind() const
	{
		glUseProgram(program);
		glDepthFunc(depth_func);
	}

	inline void change_depth_func(GLenum new_depth_func) {
		this->depth_func = new_depth_func;
	}

	inline GLenum active_depth_func() {
		return this->depth_func;
	}

	inline void add_pipeline_uniform_block(std::string block_name)
	{
		pipeline_uniform_blocks[block_name] = glGetUniformBlockIndex(program, block_name.c_str());
		if(pipeline_uniform_blocks[block_name] == GL_INVALID_INDEX){
			std::cout << "\nError: Invalid uniform block name: " << block_name << "\n";
			pipeline_uniform_blocks.erase(block_name);
		}
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
		if(pipeline_ssbo_blocks[block_name] == GL_INVALID_INDEX){
			std::cout << "\nError: Invalid shader storage block name: " << block_name << "\n";
			pipeline_ssbo_blocks.erase(block_name);
		}
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
		assert(texture.is_handle_resident());
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

	inline int get_uniform_loc(std::string uniform_name) { return glGetUniformLocation(this->program, uniform_name.c_str()); }

	template<typename T>
	void set_uniform(int uniform_loc, T value) {
		//todo error
		assert(false);
	}
	template<> void set_uniform<float>(int uniform_loc, float value) {
		bind();
		glUniform1f(uniform_loc, value);
	}
	template<> void set_uniform<double>(int uniform_loc, double value) {
		bind();
		glUniform1d(uniform_loc, value);
	}
	template<> void set_uniform<int>(int uniform_loc, int value) {
		bind();
		glUniform1i(uniform_loc, value);
	}
	template<> void set_uniform<uint32_t >(int uniform_loc, uint32_t value) {
		bind();
		glUniform1ui(uniform_loc, value);
	}
	template<> void set_uniform<uint64_t>(int uniform_loc, uint64_t value) {
		glProgramUniform1ui64ARB(program, uniform_loc, value);
	}

	template<unsigned int length, typename T>
	void set_uniform_vec(int uniform_loc, const T* value) {
		//todo error
		assert(false);
	}
	template<> void set_uniform_vec<1, float>(int uniform_loc, const float* value) { glProgramUniform1fv(program, uniform_loc, 1, value);}
	template<> void set_uniform_vec<2, float>(int uniform_loc, const float* value) { glProgramUniform2fv(program, uniform_loc, 1, value);}
	template<> void set_uniform_vec<3, float>(int uniform_loc, const float* value) { glProgramUniform3fv(program, uniform_loc, 1, value);}
	template<> void set_uniform_vec<4, float>(int uniform_loc, const float* value) { glProgramUniform4fv(program, uniform_loc, 1, value);}

	template<> void set_uniform_vec<1, double>(int uniform_loc, const double* value) { glProgramUniform1dv(program, uniform_loc, 1, value); }
	template<> void set_uniform_vec<2, double>(int uniform_loc, const double* value) { glProgramUniform2dv(program, uniform_loc, 1, value); }
	template<> void set_uniform_vec<3, double>(int uniform_loc, const double* value) { glProgramUniform3dv(program, uniform_loc, 1, value); }
	template<> void set_uniform_vec<4, double>(int uniform_loc, const double* value) { glProgramUniform4dv(program, uniform_loc, 1, value); }

	template<> void set_uniform_vec<1, int>(int uniform_loc, const int* value) { glProgramUniform1iv(program, uniform_loc, 1, value); }
	template<> void set_uniform_vec<2, int>(int uniform_loc, const int* value) { glProgramUniform2iv(program, uniform_loc, 1, value); }
	template<> void set_uniform_vec<3, int>(int uniform_loc, const int* value) { glProgramUniform3iv(program, uniform_loc, 1, value); }
	template<> void set_uniform_vec<4, int>(int uniform_loc, const int* value) { glProgramUniform4iv(program, uniform_loc, 1, value); }

	template<> void set_uniform_vec<1, uint32_t>(int uniform_loc, const uint32_t* value) { glProgramUniform1uiv(program, uniform_loc, 1, value); }
	template<> void set_uniform_vec<2, uint32_t>(int uniform_loc, const uint32_t* value) { glProgramUniform2uiv(program, uniform_loc, 1, value); }
	template<> void set_uniform_vec<3, uint32_t>(int uniform_loc, const uint32_t* value) { glProgramUniform3uiv(program, uniform_loc, 1, value); }
	template<> void set_uniform_vec<4, uint32_t>(int uniform_loc, const uint32_t* value) { glProgramUniform4uiv(program, uniform_loc, 1, value); }


	template<unsigned char n_rows, unsigned char n_columns>
	void set_uniform_mat(int uniform_loc, const float* value, bool transpose = false) {
		static_assert(true, "Attempting to set uniform with matrix of unsupported structure");
	}
	template<> void set_uniform_mat<2, 2>(int uniform_loc, const float* value, bool transpose) { glProgramUniformMatrix2fv(program, uniform_loc, 1, transpose, value); }
	template<> void set_uniform_mat<2, 3>(int uniform_loc, const float* value, bool transpose) { glProgramUniformMatrix2x3fv(program, uniform_loc, 1, transpose, value); }
	template<> void set_uniform_mat<3, 2>(int uniform_loc, const float* value, bool transpose) { glProgramUniformMatrix3x2fv(program, uniform_loc, 1, transpose, value); }
	template<> void set_uniform_mat<3, 3>(int uniform_loc, const float* value, bool transpose) { glProgramUniformMatrix3fv(program, uniform_loc, 1, transpose, value); }
	template<> void set_uniform_mat<3, 4>(int uniform_loc, const float* value, bool transpose) { glProgramUniformMatrix3x4fv(program, uniform_loc, 1, transpose, value); }
	template<> void set_uniform_mat<4, 3>(int uniform_loc, const float* value, bool transpose) { glProgramUniformMatrix4x3fv(program, uniform_loc, 1, transpose, value); }
	template<> void set_uniform_mat<4, 4>(int uniform_loc, const float* value, bool transpose) { glProgramUniformMatrix4fv(program, uniform_loc, 1, transpose, value); }


	template<unsigned char n_rows, unsigned char n_columns>
	void set_uniform_matd(int uniform_loc, const double* value, bool transpose = false) {
		static_assert(true, "Attempting to set uniform with matrix of unsupported structure");
	}
	template<> void set_uniform_matd<2, 2>(int uniform_loc, const double* value, bool transpose) { glProgramUniformMatrix2dv(program, uniform_loc, 1, transpose, value); }
	template<> void set_uniform_matd<2, 3>(int uniform_loc, const double* value, bool transpose) { glProgramUniformMatrix2x3dv(program, uniform_loc, 1, transpose, value); }
	template<> void set_uniform_matd<3, 2>(int uniform_loc, const double* value, bool transpose) { glProgramUniformMatrix3x2dv(program, uniform_loc, 1, transpose, value); }
	template<> void set_uniform_matd<3, 3>(int uniform_loc, const double* value, bool transpose) { glProgramUniformMatrix3dv(program, uniform_loc, 1, transpose, value); }
	template<> void set_uniform_matd<3, 4>(int uniform_loc, const double* value, bool transpose) { glProgramUniformMatrix3x4dv(program, uniform_loc, 1, transpose, value); }
	template<> void set_uniform_matd<4, 3>(int uniform_loc, const double* value, bool transpose) { glProgramUniformMatrix4x3dv(program, uniform_loc, 1, transpose, value); }
	template<> void set_uniform_matd<4, 4>(int uniform_loc, const double* value, bool transpose) { glProgramUniformMatrix4dv(program, uniform_loc, 1, transpose, value); }
};

class PipelineProps {
public:

	struct UniformInfo {
		GLenum type;
		bool is_block_member;
		union {
			int location;
			unsigned int block_index;
		};
	};

	struct BufferVarInfo {
		GLenum type;
		std::string name;
	};

	struct UniformBlockInfo {
		std::vector<std::string> members;
		unsigned int block_index;
	};

	struct BufferBlockInfo {
		std::vector<BufferVarInfo> members;
		unsigned int block_index;
	};

	inline UniformInfo uniform(std::string uniform) {
		return this->uniforms[uniform];
	}

	inline UniformBlockInfo uniform_block(std::string block) {
		return this->uniform_blocks[uniform_block_index[block]];
	}

	inline UniformInfo uniform_block_var(std::string block, std::string var_name) {
		auto members = uniform_block(block).members;
		auto found = members.begin();
		for(found; found < members.end(); found += 1){
			if(*found == var_name){
				break;
			}
		}
		if (found == members.end()) {
			//todo error
			throw std::make_exception_ptr("DID NOT FIND");
		}
		return uniform(var_name);
	}

	inline BufferBlockInfo ssbo_block(std::string block) {
		return this->ssbo_blocks[block];
	}

	inline BufferVarInfo ssbo_block_var(std::string block, std::string var_name) {
		for (auto& member : this->ssbo_block(block).members) {
			if (member.name == var_name)
				return member;
		}
		//todo error
		throw std::make_exception_ptr("DID NOT FIND");
	}

	PipelineProps(Pipeline& pipe) {
		get_properties(pipe);
	}

private:

	std::unordered_map<std::string, UniformInfo> uniforms;
	std::unordered_map<std::string, int> uniform_block_index;
	std::vector<UniformBlockInfo> uniform_blocks;

	std::unordered_map<std::string, BufferBlockInfo> ssbo_blocks;

	std::vector<std::string> get_interface_variable_names(unsigned int prog, GLenum iface) {
		std::vector<int> indices;
		int num_active_resources;
		glGetProgramInterfaceiv(prog, iface, GL_ACTIVE_RESOURCES, &num_active_resources);
		indices.resize(num_active_resources);
		std::iota(indices.begin(), indices.end(), 0);

		int max_name_size;
		glGetProgramInterfaceiv(prog, iface, GL_MAX_NAME_LENGTH, &max_name_size);

		std::vector<std::string> values;
		values.reserve(indices.size());
		std::vector<char> name_buff(max_name_size);
		for (const auto& i : indices) {
			glGetProgramResourceName(prog, iface, i, max_name_size, NULL, name_buff.data());
			values.push_back(std::string(name_buff.data()));
		}
		return values;
	}
	void get_properties(Pipeline& pipe) {
		unsigned int prog = pipe.get_program();

		auto ublock_names = get_interface_variable_names(prog, GL_UNIFORM_BLOCK);
		this->uniform_blocks.resize(ublock_names.size());
		for (int i = 0; i < ublock_names.size(); ++i) {
			this->uniform_block_index[ublock_names[i]] = i;
			this->uniform_blocks[i].block_index =
				glGetProgramResourceIndex(prog, GL_UNIFORM_BLOCK, ublock_names[i].c_str());
		}

		auto ssbo_block_names = get_interface_variable_names(prog, GL_SHADER_STORAGE_BLOCK);
		for (auto& name : ssbo_block_names) {
			this->ssbo_blocks[name] = BufferBlockInfo();
			this->ssbo_blocks[name].block_index =
				glGetProgramResourceIndex(prog, GL_SHADER_STORAGE_BLOCK, name.c_str());
		}

		int num_uniforms;
		glGetProgramInterfaceiv(prog, GL_UNIFORM, GL_ACTIVE_RESOURCES, &num_uniforms);

		int max_uniform_name_size;
		glGetProgramInterfaceiv(prog, GL_UNIFORM, GL_MAX_NAME_LENGTH, &max_uniform_name_size);

		std::vector<UniformInfo> values;
		values.reserve(num_uniforms);
		std::vector<char> name_buff(max_uniform_name_size);
		for (int i = 0; i < num_uniforms; ++i) {
			UniformInfo curr;
			const GLenum props[] = { GL_TYPE, GL_BLOCK_INDEX };
			int vals[2];
			glGetProgramResourceiv(prog, GL_UNIFORM, i, 2, &props[0], 2, NULL, &vals[0]);
			glGetProgramResourceName(prog, GL_UNIFORM, i, max_uniform_name_size, NULL, name_buff.data());
			std::string curr_name(name_buff.data());
			curr.type = vals[0];
			if (vals[1] == -1) {
				curr.is_block_member = false;
				curr.location = glGetUniformLocation(prog, curr_name.c_str());
			}
			else {
				curr.is_block_member = true;
				curr.block_index = static_cast<unsigned int>(vals[1]);
				this->uniform_blocks[curr.block_index].members.push_back(curr_name);
			}
			this->uniforms[curr_name] = curr;
		}

		int num_buffer_vars;
		glGetProgramInterfaceiv(prog, GL_BUFFER_VARIABLE, GL_ACTIVE_RESOURCES, &num_buffer_vars);

		int max_bvar_name_size;
		glGetProgramInterfaceiv(prog, GL_BUFFER_VARIABLE, GL_MAX_NAME_LENGTH, &max_bvar_name_size);

		values = {};
		values.reserve(num_buffer_vars);
		name_buff = std::vector<char>(max_bvar_name_size);
		for (int i = 0; i < num_buffer_vars; ++i) {
			BufferVarInfo curr;
			const GLenum props[] = { GL_TYPE, GL_BLOCK_INDEX };
			int vals[2];
			glGetProgramResourceiv(prog, GL_BUFFER_VARIABLE, i, 2, &props[0], 2, NULL, &vals[0]);
			glGetProgramResourceName(prog, GL_BUFFER_VARIABLE, i, max_bvar_name_size, NULL, name_buff.data());
			std::string curr_name(name_buff.data());
			curr.type = vals[0];
			curr.name = curr_name;
			this->ssbo_blocks[ssbo_block_names[vals[1]]].members.push_back(curr);
		}
	}
};

class _Pipeline {
private:
	unsigned int program;
	PipelineProps props;
public:
	inline unsigned int get_program(){
		return program;
	}
	_Pipeline() = default;
	static _Pipeline compute_shader(std::string source) {

	}
	static _Pipeline vert_frag_shader(std::string vs_source, std::string fs_source) {

	}

	template<typename T>
	void set_uniform(std::string uniform, T value) {
		//todo error
		assert(false);
	}
	template<> void set_uniform<float>(std::string uniform, float value) {
		glProgramUniform1f(program, props.uniform(uniform).location, value);
	}
	template<> void set_uniform<double>(std::string uniform, double value) {
		glProgramUniform1d(program, props.uniform(uniform).location, value);
	}
	template<> void set_uniform<int>(std::string uniform, int value) {
		glProgramUniform1i(program, props.uniform(uniform).location, value);
	}
	template<> void set_uniform<uint32_t>(std::string uniform, uint32_t value) {
		glProgramUniform1ui(program, props.uniform(uniform).location, value);
	}
	template<> void set_uniform<uint64_t>(std::string uniform, uint64_t value) {
		glProgramUniform1ui64ARB(program, props.uniform(uniform).location, value);
	}

	template<unsigned int length, typename T>
	void set_uniform_vec(int uniform_loc, const T* value) {
		//todo error
		assert(false);
	}
	template<> void set_uniform_vec<1, float>(int uniform_loc, const float* value) { glProgramUniform1fv(program, uniform_loc, 1, value); }
	template<> void set_uniform_vec<2, float>(int uniform_loc, const float* value) { glProgramUniform2fv(program, uniform_loc, 1, value); }
	template<> void set_uniform_vec<3, float>(int uniform_loc, const float* value) { glProgramUniform3fv(program, uniform_loc, 1, value); }
	template<> void set_uniform_vec<4, float>(int uniform_loc, const float* value) { glProgramUniform4fv(program, uniform_loc, 1, value); }

	template<> void set_uniform_vec<1, double>(int uniform_loc, const double* value) { glProgramUniform1dv(program, uniform_loc, 1, value); }
	template<> void set_uniform_vec<2, double>(int uniform_loc, const double* value) { glProgramUniform2dv(program, uniform_loc, 1, value); }
	template<> void set_uniform_vec<3, double>(int uniform_loc, const double* value) { glProgramUniform3dv(program, uniform_loc, 1, value); }
	template<> void set_uniform_vec<4, double>(int uniform_loc, const double* value) { glProgramUniform4dv(program, uniform_loc, 1, value); }

	template<> void set_uniform_vec<1, int>(int uniform_loc, const int* value) { glProgramUniform1iv(program, uniform_loc, 1, value); }
	template<> void set_uniform_vec<2, int>(int uniform_loc, const int* value) { glProgramUniform2iv(program, uniform_loc, 1, value); }
	template<> void set_uniform_vec<3, int>(int uniform_loc, const int* value) { glProgramUniform3iv(program, uniform_loc, 1, value); }
	template<> void set_uniform_vec<4, int>(int uniform_loc, const int* value) { glProgramUniform4iv(program, uniform_loc, 1, value); }

	template<> void set_uniform_vec<1, uint32_t>(int uniform_loc, const uint32_t* value) { glProgramUniform1uiv(program, uniform_loc, 1, value); }
	template<> void set_uniform_vec<2, uint32_t>(int uniform_loc, const uint32_t* value) { glProgramUniform2uiv(program, uniform_loc, 1, value); }
	template<> void set_uniform_vec<3, uint32_t>(int uniform_loc, const uint32_t* value) { glProgramUniform3uiv(program, uniform_loc, 1, value); }
	template<> void set_uniform_vec<4, uint32_t>(int uniform_loc, const uint32_t* value) { glProgramUniform4uiv(program, uniform_loc, 1, value); }


	template<unsigned char n_rows, unsigned char n_columns>
	void set_uniform_mat(int uniform_loc, const float* value, bool transpose) {
		static_assert(true, "Attempting to set uniform with matrix of unsupported structure");
	}
	template<> void set_uniform_mat<2, 2>(int uniform_loc, const float* value, bool transpose) { glProgramUniformMatrix2fv(program, uniform_loc, 1, transpose, value); }
	template<> void set_uniform_mat<2, 3>(int uniform_loc, const float* value, bool transpose) { glProgramUniformMatrix2x3fv(program, uniform_loc, 1, transpose, value); }
	template<> void set_uniform_mat<3, 2>(int uniform_loc, const float* value, bool transpose) { glProgramUniformMatrix3x2fv(program, uniform_loc, 1, transpose, value); }
	template<> void set_uniform_mat<3, 3>(int uniform_loc, const float* value, bool transpose) { glProgramUniformMatrix3fv(program, uniform_loc, 1, transpose, value); }
	template<> void set_uniform_mat<3, 4>(int uniform_loc, const float* value, bool transpose) { glProgramUniformMatrix3x4fv(program, uniform_loc, 1, transpose, value); }
	template<> void set_uniform_mat<4, 3>(int uniform_loc, const float* value, bool transpose) { glProgramUniformMatrix4x3fv(program, uniform_loc, 1, transpose, value); }
	template<> void set_uniform_mat<4, 4>(int uniform_loc, const float* value, bool transpose) { glProgramUniformMatrix4fv(program, uniform_loc, 1, transpose, value); }


	template<unsigned char n_rows, unsigned char n_columns>
	void set_uniform_matd(int uniform_loc, const double* value, bool transpose = false) {
		static_assert(true, "Attempting to set uniform with matrix of unsupported structure");
	}
	template<> void set_uniform_matd<2, 2>(int uniform_loc, const double* value, bool transpose) { glProgramUniformMatrix2dv(program, uniform_loc, 1, transpose, value); }
	template<> void set_uniform_matd<2, 3>(int uniform_loc, const double* value, bool transpose) { glProgramUniformMatrix2x3dv(program, uniform_loc, 1, transpose, value); }
	template<> void set_uniform_matd<3, 2>(int uniform_loc, const double* value, bool transpose) { glProgramUniformMatrix3x2dv(program, uniform_loc, 1, transpose, value); }
	template<> void set_uniform_matd<3, 3>(int uniform_loc, const double* value, bool transpose) { glProgramUniformMatrix3dv(program, uniform_loc, 1, transpose, value); }
	template<> void set_uniform_matd<3, 4>(int uniform_loc, const double* value, bool transpose) { glProgramUniformMatrix3x4dv(program, uniform_loc, 1, transpose, value); }
	template<> void set_uniform_matd<4, 3>(int uniform_loc, const double* value, bool transpose) { glProgramUniformMatrix4x3dv(program, uniform_loc, 1, transpose, value); }
	template<> void set_uniform_matd<4, 4>(int uniform_loc, const double* value, bool transpose) { glProgramUniformMatrix4dv(program, uniform_loc, 1, transpose, value); }

};


class ComputeShader
{
private:
	unsigned int program;
	std::unordered_map<std::string, unsigned int> ssbo_location;
public:
	ComputeShader() = default;

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
	
	inline void destroy() {
		glDeleteProgram(program);
		program = 0;
		ssbo_location.clear();
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

static const std::string debug_cube_vertex_shader_file = "src\\Graphics\\DebugCubeVertex.glsl";
static const std::string debug_cube_fragment_shader_file = "src\\Graphics\\DebugCubeFragment.glsl";

class DebugCube {
private:
	Pipeline curr_pipeline;

	int projection_mat_uniform_loc;
	int view_mat_uniform_loc;
	int model_mat_uniform_loc;
	std::array<float, 4 * 4> model_mat;
	std::array<float, 4 * 4> view_mat;
	std::array<float, 4 * 4> proj_mat;

	void update_uniform_locs() {
		this->projection_mat_uniform_loc =
			curr_pipeline.get_uniform_loc("projection_matrix");

		this->view_mat_uniform_loc =
			curr_pipeline.get_uniform_loc("view_matrix");

		this->model_mat_uniform_loc = 
			curr_pipeline.get_uniform_loc("model_matrix");
	}

	void reset_uniforms() {
		curr_pipeline.set_uniform_mat<4, 4>(projection_mat_uniform_loc, proj_mat.data());
		curr_pipeline.set_uniform_mat<4, 4>(view_mat_uniform_loc, view_mat.data());
		curr_pipeline.set_uniform_mat<4, 4>(model_mat_uniform_loc, model_mat.data());

		//glUniformMatrix4fv(this->projection_mat_uniform_loc, 1, GL_FALSE, this->proj_mat.data());
		//glUniformMatrix4fv(this->view_mat_uniform_loc, 1, GL_FALSE, this->view_mat.data());
		//glUniformMatrix4fv(this->model_mat_uniform_loc, 1, GL_FALSE, this->model_mat.data());
	}


public:
	DebugCube(const float* projection_mat) : curr_pipeline(
		Pipeline::create_from_source(
			std::string(Pipeline::read_shader_file(std::string(debug_cube_vertex_shader_file))),
			Pipeline::read_shader_file(std::string(debug_cube_fragment_shader_file)))
	) {
		update_uniform_locs();
		change_projection_mat(projection_mat);
		constexpr float identity_mat4[16] = {
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		};
		change_view_mat(identity_mat4);
		change_model_mat(identity_mat4);
	}

	void change_fragment_shader(std::string fragment_shader_src) {
		curr_pipeline.destroy();
		curr_pipeline = Pipeline::create_from_source(
			Pipeline::read_shader_file(std::string(debug_cube_vertex_shader_file)),
			fragment_shader_src);
		update_uniform_locs();
		reset_uniforms();
	}

	void change_projection_mat(const float* proj_mat_in) {
		memcpy(this->proj_mat.data(), proj_mat_in, sizeof(float) * 16);
		curr_pipeline.bind();
		glUniformMatrix4fv(this->projection_mat_uniform_loc, 1, GL_FALSE, this->proj_mat.data());
	}

	void change_view_mat(const float* view_mat_in) {
		memcpy(this->view_mat.data(), view_mat_in, sizeof(float) * 16);
		curr_pipeline.bind();
		glUniformMatrix4fv(this->view_mat_uniform_loc, 1, GL_FALSE, this->view_mat.data());
	}

	void change_model_mat(const float * model_mat_in) {
		memcpy(this->model_mat.data(), model_mat_in, sizeof(float) * 16);
		curr_pipeline.bind();
		glUniformMatrix4fv(this->model_mat_uniform_loc, 1, GL_FALSE, this->model_mat.data());
	}

	void draw(float* transform_mat = nullptr) {
		if (transform_mat != nullptr) {
			change_model_mat(transform_mat);
			//above function also binds the pipeline
		}
		else {
			curr_pipeline.bind();
		}
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}
};


static const std::string fullscreen_vert_file = "src/Graphics/FullScreenVert.glsl";
static const std::string debug_tex_draw_frag_file = "src/Graphics/DebugTexDrawFrag.glsl";

class DebugTextureDrawer {
private:
	Pipeline pipe;
public:
	DebugTextureDrawer() : pipe(Pipeline(std::string(fullscreen_vert_file), std::string(debug_tex_draw_frag_file))) {
		pipe.bind();
		pipe.set_uniform<int>(pipe.get_uniform_loc("tex"), 0);
	}
	void operator() (Texture_2D& tex) {
		pipe.bind();
		tex.bind_to_unit(GL_TEXTURE_2D, 0);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
	void operator() (Texture_3D& tex) {
		pipe.bind();
		tex.bind_to_unit(GL_TEXTURE_3D, 0);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
	template<typename T>
	void set_uniform(std::string uniform_name, T value) {
		pipe.set_uniform<T>(pipe.get_uniform_loc(uniform_name), value);
	}
	template<typename T, unsigned int length>
	void set_uniform_vec(std::string uniform_name, T* value) {
		pipe.set_uniform_vec<length, T>(pipe.get_uniform_loc(uniform_name), value);
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