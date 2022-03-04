#version 450 core
#extension GL_ARB_bindless_texture : enable


in vec2 texcoords;

layout(std430,binding = 7) buffer Materials
{
  vec4 materials[];
};

flat in uint mat_id;

layout(bindless_sampler) uniform;
uniform gbuffer{
	sampler2D gbuff[2];
};

out vec4 fragcolor;

void main(){
    vec3 lightdir = vec3(0.7,0.45,0.3);
	float coeff = max(dot(lightdir,texture(gbuff[1],texcoords).rgb),0.0);
	fragcolor = vec4(materials[mat_id].rgb*coeff,1);
}