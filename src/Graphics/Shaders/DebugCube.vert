#version 450 core

float uv_vals[12] = {
    0.0, 0.0,
    1.0, 0.0,
    1.0, 1.0,
    1.0, 1.0,
    0.0, 1.0,
    0.0, 0.0
};

out vec3 frag_pos;
out vec3 normal;
out vec2 uv;

uniform mat4 projection_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;

void main()
{
    int tri = gl_VertexID / 3;
    int idx = gl_VertexID % 3;
    int face = tri / 2;
    int top = tri % 2;

    int dir = face % 3;
    int pos = face / 3;

    int nz = dir >> 1;
    int ny = dir & 1;
    int nx = 1 ^ (ny | nz);

    vec3 d = vec3(nx, ny, nz);
    float flip = 1 - 2 * pos;

    vec3 n = flip * d;
    vec3 u = -d.yzx;
    vec3 v = flip * d.zxy;

    float mirror = -1 + 2 * top;
    vec3 xyz = n + mirror*(1-2*(idx&1))*u + mirror*(1-2*(idx>>1))*v;
    frag_pos = vec3(model_matrix * vec4(xyz, 1.0));
    normal = mat3(transpose(inverse(model_matrix))) * n;
    uv = vec2(uv_vals[3*top + idx]);
    gl_Position = projection_matrix * view_matrix * vec4(frag_pos, 1.0);
}