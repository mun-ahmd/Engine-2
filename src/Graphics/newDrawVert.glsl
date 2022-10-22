#version 450 core
#extension GL_ARB_shader_draw_parameters : enable
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;
layout (location = 2) in vec2 aUV;

layout (std140) uniform Matrices
{
    mat4 projection;
    mat4 view;
};

layout (std430) buffer Transformations
{
    mat4 models[];
};

layout (std430) buffer MaterialIds
{
    uint material_ids[];
};

out vec3 normal;
out vec2 uv;
out vec3 frag_pos;
out flat uint mat_id;

void main()
{
    mat4 model = models[gl_BaseInstanceARB];
    gl_Position = projection * view * model * vec4(aPos.x, aPos.y, aPos.z, 1.0);
	normal = normalize(transpose(inverse(mat3(model))) * aNorm);
    uv = aUV;
    mat_id = material_ids[gl_BaseInstanceARB];
    frag_pos = (model * vec4(aPos,1)).xyz;
}