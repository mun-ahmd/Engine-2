#version 450 core
#extension GL_ARB_bindless_texture : enable


in vec2 texcoords;

layout(std430, binding=7) buffer Materials
{
  vec4 materials[];
};

flat in uint mat_id;

layout(std140) uniform gbuffer{
	sampler2D gpos;
	sampler2D gnorm;
};

layout(bindless_sampler) uniform sampler2D shadow_map;	
uniform mat4 lightspace;

uniform vec3 camera_pos;

out vec4 fragcolor;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadow_map, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z * 2 - 1;
    // check whether current frag pos is in shadow
    float bias = 0.00001;
    float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;  
	return shadow;
} 

uniform vec3 lightPos;

float LinearizeDepth(float depth, float zNear, float zFar)
{
	//formula to linearize depth:
	return (2*zNear ) / (zFar + zNear - depth*(zFar -zNear)) ;
}

void main(){
    vec3 lightdir = normalize(-lightPos);
	vec3 lightcolor = materials[mat_id -1].rgb;
	fragcolor = vec4(lightcolor, 1);
	return;
	//TODO MAJOR CHECK WHY NORMALS ARE COMING REVERSED FROM CPU SIDE
	// vec3 norm = normalize(-texture(gnorm, texcoords).rgb);
	// vec3 frag_pos = texture(gpos, texcoords).rgb;
	// vec4 lightspace_coords = (lightspace * vec4(frag_pos, 1.0));
	// float shadow = 1.0 - ShadowCalculation(lightspace_coords);

	// float diff = (max(dot(lightdir, norm), 0.0)) * shadow + 0.09;

	// vec3 view_dir = normalize(camera_pos - frag_pos);
	// vec3 reflect_dir = reflect(-lightdir, norm);
	// float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32) * shadow;

	// fragcolor = vec4(lightcolor * (diff+spec), 1);

//	fragcolor = vec4(vec3(10000 * LinearizeDepth(shadow, 0.001, 100.0)), 1);
//	fragcolor = vec4(vec3(1 * LinearizeDepth(texture(shadow_map, texcoords).r, 0.001, 100.0)), 1);
}