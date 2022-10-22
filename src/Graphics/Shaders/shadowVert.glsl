#version 450 core

layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 shadow_lightspace;

void main(){
	gl_Position = shadow_lightspace * model * vec4(aPos, 1.0);
};