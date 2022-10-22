#version 450 core
#extension GL_ARB_bindless_texture : enable
out vec4 fragcolor;
in vec2 texcoords; // texcoords are in the normalized [0,1] range for the viewport-filling quad part of the triangle
uniform sampler2D tex;

float LinearizeDepth(float depth, float zNear, float zFar);
uniform vec2 near_far;

void main() {
	//fragcolor = texture(tex, texcoords);
	fragcolor = vec4(vec3(LinearizeDepth(texture(tex, texcoords).r, near_far.x , near_far.y)), 1.0);
}

// required when using a perspective projection matrix
float LinearizeDepth(float depth, float zNear, float zFar)
{
	//formula to linearize depth:
	return (2*zNear ) / (zFar + zNear - depth*(zFar -zNear)) ;
}