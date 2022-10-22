#include "StaticPipelinesLib.h"

#define NUM_PIPES 1
static std::array<unsigned int, NUM_PIPES> pipeline_programs;

void set_program(unsigned int pipe_id, unsigned int program) {
	pipeline_programs[pipe_id] = program;
}

unsigned int get_program(unsigned int pipe_id) {
	return pipeline_programs[pipe_id];
}
