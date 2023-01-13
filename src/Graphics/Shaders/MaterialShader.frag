#version 450 core
#extension GL_ARB_bindless_texture : enable

in vec2 texcoords;

layout(std140) uniform Materials {
  sampler2D baseColor;                   //	8				0
  sampler2D metallicRoughness;           //	8				8
  sampler2D normalMap;                   //	8				16
  sampler2D garbage;                     //	8				24
  vec4 metallicFac_roughnessFac_garbage; //	16      32
  vec4 baseColorFac;                     //	16			48
};

flat in uint mat_id;
out vec4 fragcolor;

void main() {
   fragcolor = texture(baseColor, texcoords);
  //  fragcolor = vec4(1.0);
 }