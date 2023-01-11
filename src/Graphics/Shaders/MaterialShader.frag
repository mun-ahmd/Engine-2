#version 450 core
#extension GL_ARB_bindless_texture : enable


in vec2 texcoords;

layout(std430, binding=7) buffer Materials
{
  vec4 materials[];
};

flat in uint mat_id;
out vec4 fragcolor;

void main(){
	fragcolor = vec4(materials[mat_id -1].rgb, 1);
}