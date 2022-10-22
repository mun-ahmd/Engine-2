#version 450 core

in vec3 normal;

out vec4 fragcolor;

void main() {
	fragcolor = vec4(vec3(0.1 + max(dot(normal, normalize(vec3(10, 2, 5))), 0.0)), 1.0);
}