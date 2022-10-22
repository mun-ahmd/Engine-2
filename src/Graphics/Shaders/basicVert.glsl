#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out VS_OUT
{
    vec3 frag_pos;
    vec4 shadow_view_pos;
    vec3 norm;
    vec2 uv;
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 shadow_lightspace;

void main(){
	vs_out.norm = normalize(transpose(inverse(mat3(model))) * aNormal);
    vs_out.uv = aTexCoord;
    vs_out.frag_pos = (model * vec4(aPos,1)).xyz;
    gl_Position = projection * view * vec4(vs_out.frag_pos, 1);
    vs_out.shadow_view_pos = (shadow_lightspace * vec4(vs_out.frag_pos, 1));
};