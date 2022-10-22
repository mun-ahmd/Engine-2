#pragma once
#include <array>
#include "glad/glad.h"


inline void set_uniform(unsigned int program, int location, float val) { glProgramUniform1f(program, location, val); }
inline void set_uniform(unsigned int program, int location, double val) { glProgramUniform1d(program, location, val); }
inline void set_uniform(unsigned int program, int location, int val) { glProgramUniform1i(program, location, val); }
inline void set_uniform(unsigned int program, int location, unsigned int val) { glProgramUniform1ui(program, location, val); }
inline void set_uniform(unsigned int program, int location, uint64_t val) { glProgramUniform1ui64ARB(program, location, val); }


typedef void(*UniformFloatVecFunc)(GLuint, GLint, GLsizei, const GLfloat*);
inline static const UniformFloatVecFunc fvec_uni_funcs[4] = {glProgramUniform1fv, glProgramUniform2fv, glProgramUniform3fv, glProgramUniform4fv};

typedef void(*UniformDoubleVecFunc)(GLuint, GLint, GLsizei, const GLdouble*);
inline static const UniformDoubleVecFunc dvec_uni_funcs[4] = { glProgramUniform1dv, glProgramUniform2dv, glProgramUniform3dv, glProgramUniform4dv };

typedef void(*UniformIntVecFunc)(GLuint, GLint, GLsizei, const GLint*);
inline static const UniformIntVecFunc ivec_uni_funcs[4] = { glProgramUniform1iv, glProgramUniform2iv, glProgramUniform3iv, glProgramUniform4iv };

typedef void(*UniformUintVecFunc)(GLuint, GLint, GLsizei, const GLuint*);
inline static const UniformUintVecFunc uivec_uni_funcs[4] = { glProgramUniform1uiv, glProgramUniform2uiv, glProgramUniform3uiv, glProgramUniform4uiv };


template<unsigned int len>
inline void set_uniform_vec(unsigned int program, int location, const float* val) { fvec_uni_funcs[len - 1](program, location, val); }
template<unsigned int len>
inline void set_uniform_vec(unsigned int program, int location, const double* val) { fvec_uni_funcs[len - 1](program, location, val); }
template<unsigned int len>
inline void set_uniform_vec(unsigned int program, int location, const int* val) { ivec_uni_funcs[len - 1](program, location, val); }
template<unsigned int len>
inline void set_uniform_vec(unsigned int program, int location, const uint32_t* val) { uivec_uni_funcs[len - 1](program, location, val); }

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


template<unsigned char n_rows, unsigned char n_columns>
void set_uniform_mat(unsigned int program, int uniform_loc, const float* value, bool transpose = false) {
	static_assert((n_rows < 5) && (n_columns < 5) && (n_rows > 1) && (n_columns > 1), "Attempting to set uniform with matrix of unsupported structure");
	uniform_float_mat_funcs[4 * (n_columns - 1) + n_rows - 1](
		program, uniform_loc, 1, transpose, value
		);
}

template<unsigned char n_rows, unsigned char n_columns>
void set_uniform_mat(unsigned int program, int uniform_loc, const double* value, bool transpose = false) {
	static_assert((n_rows < 5) && (n_columns < 5) && (n_rows > 1) && (n_columns > 1));
	uniform_double_mat_funcs[4 * (n_columns - 1) + n_rows - 1](
		program, uniform_loc, 1, transpose, value
		);
}

unsigned int create_program(const char* vertex_path, const char* fragment_path);
void set_program(unsigned int pipe_id, unsigned int program);
unsigned int get_program(unsigned int pipe_id);

template<typename T, unsigned int pipe_id>
class Uniform {
private:
	int location;
public:
	Uniform() = default;

	void init(const char * name) {
		glGetUniformLocation(get_program(pipe_id), name);
	}
	void set(T value) {
		set_uniform(get_program(pipe_id), location, value);
	}
};

template<typename T, unsigned int len, unsigned int pipe_id>
class UniformVec {
private:
	int location;
public:
	UniformVec() = default;

	void init(const char* name) {
		glGetUniformLocation(get_program(pipe_id), name);
	}
	void set(const T* value) {
		set_uniform_vec<len>(get_program(pipe_id), location, value);
	}
};

template<typename T, unsigned char rows, unsigned char columns, unsigned int pipe_id>
class UniformMat {
private:
	int location;
public:
	UniformMat() = default;

	void init(const char* name) {
		glGetUniformLocation(get_program(pipe_id), name);
	}
	void set(const T* value, bool transpose = false) {
		set_uniform_mat<rows, columns>(get_program(pipe_id), location, value, transpose);
	}
};


