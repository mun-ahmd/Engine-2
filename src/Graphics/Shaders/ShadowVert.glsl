#version 450 core
#extension GL_ARB_shader_draw_parameters : enable
layout (location = 0) in vec3 aPos;

layout (std140) uniform Matrices
{
    mat4 projection;
    mat4 view;
};

layout (std430) buffer Transformations
{
    mat4 models[];
};

uniform mat4 lightspace;

void main()
{
    mat4 model = models[gl_BaseInstanceARB];
    gl_Position = lightspace * model * vec4(aPos.x, aPos.y, aPos.z, 1.0);
}