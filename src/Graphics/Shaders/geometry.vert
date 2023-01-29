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

out flat uint mat_id;
out flat uint cluster_id;

void main()
{
    mat4 model = models[gl_BaseInstanceARB];
    gl_Position = projection * view * model * vec4(aPos.x, aPos.y, aPos.z, 1.0);
    vec3 interpolations_verts[3] = vec3[3](vec3(1,0,0), vec3(0,1,0), vec3(0,0,1));
    mat_id = material_ids[gl_BaseInstanceARB];
    cluster_id = uint(gl_DrawIDARB);
}