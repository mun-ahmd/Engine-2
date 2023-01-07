#version 450 core
#extension GL_ARB_bindless_texture : enable

layout(bindless_sampler) uniform;

uniform sampler2D shadow_map;

in VS_OUT
{
    vec3 frag_pos;
    vec4 shadow_view_pos;
    vec3 norm;
    vec2 uv;
} fs_in;

out vec4 fragColor;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadow_map, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
    float bias = 0.00001;
    float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;  

    return shadow;
}  

void main()
{
    float factor = max(dot(normalize(vec3(6, 8, 0)), fs_in.norm), 0);
    float isShadow = ShadowCalculation(fs_in.shadow_view_pos);
    fragColor = vec4(( 0.1 + factor * (1-isShadow)) * vec3(1.0, 1.0, 1.0), 1);
}