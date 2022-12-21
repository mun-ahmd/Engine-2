#version 450 core
#extension GL_ARB_bindless_texture : enable

//layout(bindless_sampler) uniform sampler2D albedo;

layout (location = 0) out vec3 g_position;
layout (location = 1) out vec3 g_normal;
layout (location = 2) out float material_depth;

in vec3 frag_pos;
in vec3 normal;
in vec2 uv;
in flat uint mat_id;

float normalizeduint(uint val){
	return val/(pow(2,32)-1);
};

void main()
{
	g_normal = normalize(normal);
	g_position = frag_pos;

	material_depth = normalizeduint(mat_id);
};