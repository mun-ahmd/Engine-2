#version 450 core

out vec2 texcoords; // texcoords are in the normalized [0,1] range for the viewport-filling quad part of the triangle

layout(std140) uniform MaterialID{
    uint material_id;
};

out flat uint mat_id;
void main() {
        vec2 vertices[3]=vec2[3](vec2(-1,-1), vec2(3,-1), vec2(-1, 3));
        gl_Position = vec4(vertices[gl_VertexID],(material_id/(pow(2,32)-1)),1);
        texcoords = 0.5 * gl_Position.xy + vec2(0.5);
        mat_id = material_id;
}