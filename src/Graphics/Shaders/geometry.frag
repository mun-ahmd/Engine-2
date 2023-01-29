#version 450 core
#extension GL_ARB_bindless_texture : enable

// layout(bindless_sampler) uniform sampler2D albedo;

layout(location = 0) out uvec2 visibility;
layout(location = 1) out float material_depth;

in flat uint mat_id;
in flat uint cluster_id;

float normalizeduint(uint val) { return val / (pow(2, 32) - 1); }

void main() {
  uint tri_id = (cluster_id << 8) | ((gl_PrimitiveID << 1) | 1);
  visibility = uvec2(tri_id, floatBitsToUint(gl_FragCoord.z));
  material_depth = normalizeduint(mat_id);
}