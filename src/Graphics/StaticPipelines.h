#pragma once
#include "StaticPipelinesLib.h"
#include <array>


#define pipe0members Uniform<int, 0> model; Uniform<int, 0> view;
#define pipe0init uniforms.model.init("model"); uniforms.view.init("view");

class StaticPipelines {
public:
	struct {
		struct {
			pipe0members
		} uniforms;
		void init() {
			pipe0init
		}
	} pipe0; 

};

//todo finish static pipelines
/*
* so create defines named after the pipeline:
*	this enables us to easily change the members or the initialization of a pipeline without needing to make changes in the class directly
*/