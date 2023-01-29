#version 450 core
#extension GL_ARB_bindless_texture : enable

in vec2 texcoords;

struct IndirectDrawData {
  uint count;         // would equal number of triangles per cluster * 3
  uint instanceCount; // setting it to 1 for now..
  uint firstIndex;    // number of indices before the first one of this
                      // cluster regardless of which mesh it belongs to
  uint baseVertex;    // number of vertices before the first one of
                      // cluster's mesh
  uint baseInstance;  // set it to mesh index for now..
};

struct Vertex3 {
  vec4 posxyz_normx;
  vec4 normyz_uv;
};

uniform vec2 width_height;

layout(bindless_sampler) uniform usampler2D visibility_tex;

layout(std140) uniform Materials {
  sampler2D baseColor;                   //	8				0
  sampler2D metallicRoughness;           //	8				8
  sampler2D normalMap;                   //	8				16
  sampler2D garbage;                     //	8				24
  vec4 metallicFac_roughnessFac_garbage; //	16      32
  vec4 baseColorFac;                     //	16			48
};

layout(std430) buffer IndirectDrawBuffer { IndirectDrawData indirect_draw[]; };

layout(std430) buffer IndicesData { uint indices[]; };

layout(std430) buffer VertexData { Vertex3 vertices[]; };

layout(std140) uniform Matrices {
  mat4 projection;
  mat4 view;
};

layout(std430) buffer Transformations { mat4 models[]; };

flat in uint mat_id;

layout(location = 0) out vec4 albedo;
layout(location = 1) out vec3 position;
layout(location = 2) out vec3 normal;

vec4 worldSpace(float depth) {
  vec4 view_space =
      inverse(projection) * (vec4(vec3(texcoords, depth) * 2.0 - 1.0, 1.0));
  view_space /= view_space.w;
  return inverse(view) * view_space;
}

vec3 worldToClipSpace(vec4 world) {
  vec4 view_space = view * world;
  vec4 clip = projection * view_space;
  float invw = 1 / clip.w;
  return vec3(clip.xy * invw, 1.0 / view_space.z);
}

struct BarycentricDeriv {
  vec3 m_lambda;
  vec3 m_ddx;
  vec3 m_ddy;
};

BarycentricDeriv CalcFullBary(vec4 pt0, vec4 pt1, vec4 pt2, vec2 pixelNdc,
                              vec2 winSize) {
  BarycentricDeriv ret;

  vec3 invW = 1.0 / (vec3(pt0.w, pt1.w, pt2.w));

  vec2 ndc0 = pt0.xy * invW.x;
  vec2 ndc1 = pt1.xy * invW.y;
  vec2 ndc2 = pt2.xy * invW.z;

  float invDet = 1.0 / (determinant(mat2x2(ndc2 - ndc1, ndc0 - ndc1)));
  ret.m_ddx =
      vec3(ndc1.y - ndc2.y, ndc2.y - ndc0.y, ndc0.y - ndc1.y) * invDet * invW;
  ret.m_ddy =
      vec3(ndc2.x - ndc1.x, ndc0.x - ndc2.x, ndc1.x - ndc0.x) * invDet * invW;
  float ddxSum = dot(ret.m_ddx, vec3(1, 1, 1));
  float ddySum = dot(ret.m_ddy, vec3(1, 1, 1));

  vec2 deltaVec = pixelNdc - ndc0;
  float interpInvW = invW.x + deltaVec.x * ddxSum + deltaVec.y * ddySum;
  float interpW = 1.0 / (interpInvW);

  ret.m_lambda.x =
      interpW * (invW[0] + deltaVec.x * ret.m_ddx.x + deltaVec.y * ret.m_ddy.x);
  ret.m_lambda.y =
      interpW * (0.0 + deltaVec.x * ret.m_ddx.y + deltaVec.y * ret.m_ddy.y);
  ret.m_lambda.z =
      interpW * (0.0 + deltaVec.x * ret.m_ddx.z + deltaVec.y * ret.m_ddy.z);

  ret.m_ddx *= (2.0 / winSize.x);
  ret.m_ddy *= (2.0 / winSize.y);
  ddxSum *= (2.0 / winSize.x);
  ddySum *= (2.0 / winSize.y);

  ret.m_ddy *= -1.0;
  ddySum *= -1.0;

  float interpW_ddx = 1.0 / (interpInvW + ddxSum);
  float interpW_ddy = 1.0 / (interpInvW + ddySum);

  ret.m_ddx =
      interpW_ddx * (ret.m_lambda * interpInvW + ret.m_ddx) - ret.m_lambda;
  ret.m_ddy =
      interpW_ddy * (ret.m_lambda * interpInvW + ret.m_ddy) - ret.m_lambda;

  return ret;
}

vec3 InterpolateWithDeriv(BarycentricDeriv deriv, float v0, float v1,
                          float v2) {
  vec3 mergedV = vec3(v0, v1, v2);
  vec3 ret;
  ret.x = dot(mergedV, deriv.m_lambda);
  ret.y = dot(mergedV, deriv.m_ddx);
  ret.z = dot(mergedV, deriv.m_ddy);
  return ret;
}

void main() {
  uint id = texture(visibility_tex, texcoords).r;
  uint cluster_id = id >> 8;
  uint tri_id = (id << 24) >> 25;
  uint flag = id & 1;

  IndirectDrawData indirect = indirect_draw[cluster_id];
  uint index = indirect.firstIndex + tri_id * 3;
  uint baseVertex = indirect.baseVertex;

  mat4 model = models[indirect.baseInstance];
  Vertex3 vert0 = vertices[baseVertex + indices[index]];
  vec4 world0 = model * vec4(vert0.posxyz_normx.xyz, 1);
  Vertex3 vert1 = vertices[baseVertex + indices[index + 1]];
  vec4 world1 = model * vec4(vert1.posxyz_normx.xyz, 1);
  Vertex3 vert2 = vertices[baseVertex + indices[index + 2]];
  vec4 world2 = model * vec4(vert2.posxyz_normx.xyz, 1);

  BarycentricDeriv bderiv = CalcFullBary(
      projection * view * world0, projection * view * world1,
      projection * view * world2, texcoords * 2.0 - 1.0, width_height);

  vec3 uv_interps[2] = {
      InterpolateWithDeriv(bderiv, vert0.normyz_uv.z, vert1.normyz_uv.z,
                           vert2.normyz_uv.z),
      InterpolateWithDeriv(bderiv, vert0.normyz_uv.w, vert1.normyz_uv.w,
                           vert2.normyz_uv.w)};

  albedo.rgb = (baseColorFac * textureGrad(baseColor, vec2(uv_interps[0].x, uv_interps[1].x),
                           vec2(uv_interps[0].y, uv_interps[1].y),
                           vec2(uv_interps[0].z, uv_interps[1].z))).rgb;

  position.rgb = bderiv.m_lambda.x * world0.xyz +
                 bderiv.m_lambda.y * world1.xyz +
                 bderiv.m_lambda.z * world2.xyz;

  normal.rgb =
      bderiv.m_lambda.x * vec3(vert0.posxyz_normx.w, vert0.normyz_uv.xy) +
      bderiv.m_lambda.y * vec3(vert1.posxyz_normx.w, vert1.normyz_uv.xy) +
      bderiv.m_lambda.z * vec3(vert2.posxyz_normx.w, vert2.normyz_uv.xy);
}
