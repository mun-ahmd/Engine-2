#version 450 core
#extension GL_ARB_bindless_texture : enable

// layout(bindless_sampler) uniform sampler2D albedo;

layout(location = 0) out uvec2 visibility;
layout(location = 1) out vec3 interpolate;
layout(location = 2) out float material_depth;

in vec3 inter;
in flat uint mat_id;
in flat uint cluster_id;
in vec4 view_space;

float normalizeduint(uint val) { return val / (pow(2, 32) - 1); }

void main() {
  uint tri_id = (cluster_id << 8) | ((gl_PrimitiveID << 1) | 1);
  visibility = uvec2(tri_id, floatBitsToUint(1.0/view_space.z));
  interpolate = inter;
  material_depth = normalizeduint(mat_id);
}