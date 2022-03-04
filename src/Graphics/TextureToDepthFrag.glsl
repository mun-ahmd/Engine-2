#version 450 core
#extension GL_ARB_bindless_texture : enable

in vec2 texcoords;
layout(bindless_sampler) uniform sampler2D depth_texture;

void main(){
	gl_FragDepth = texture(depth_texture,texcoords).r;
}