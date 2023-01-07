#version 450 core
#extension GL_ARB_bindless_texture : enable

out vec4 fragcolor;
in vec2 texcoords;
flat in uint mat_id;

layout(std140) uniform gbuffer {
  sampler2D gpos;
  sampler2D gnorm;
  sampler2D albedo;
};

struct PointLight {
  vec4 position_radius;
  vec4 color_intensity;
};

layout(std430) buffer point_lights_buffer {
  int num_point_lights;
  PointLight point_lights[];
};

float LinearizeDepth(float depth, float zNear, float zFar) {
  // formula to linearize depth:
  return (2 * zNear) / (zFar + zNear - depth * (zFar - zNear));
}

vec3 processPointLight(PointLight light, vec3 normal, vec3 pos) {
  vec3 lightdir = light.position_radius.xyz - pos;
  float radius = light.position_radius.w;
  float radius2 = radius * radius;
  float s_2 = min(dot(lightdir, lightdir), radius2) / radius2;
  float fac = 1.0 - s_2;
  float A = light.color_intensity.a;
  float F = 1.0;
  float attenuation = A * fac * fac / (1.0 + F * s_2);
  return light.color_intensity.rgb *
         max(dot(normalize(lightdir), normal), 0.0) * attenuation;
}

void main() {
  vec3 normal = normalize(texture(gnorm, texcoords).rgb);
  vec3 pos = texture(gpos, texcoords).rgb;
  vec3 diffuse = vec3(0.0);
  vec3 albedo = texture(albedo, texcoords).rgb;
  for(int i = 0; i < num_point_lights; i++){
    diffuse +=  albedo * processPointLight(point_lights[i], normal, pos);
  }
  fragcolor = vec4(diffuse, 1.0);
}
