#version 450 core
layout(local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

struct Vertex	//weird store to prevent allignment issues
{
	vec4 pos_normX;
	vec4 normYZ_uv;
	vec3 pos()
	{
		return pos_normX.xyz;
	}
	vec3 norm()
	{
		return vec3(pos_normX.w,normYZ_uv.xy);
	}
	vec2 uv()
	{
		return vec2(normYZ_uv.zw);
	}
};

struct Triangle
{
	Vertex one;
	Vertex two;
	Vertex three;
};

struct Cluster	//no padding required
{
	uvec4 meshid_numinstances_numclusters_padding;
	Triangle triangles[128];
};

layout(std430, binding = 3) buffer Clusters
{
	Cluster clusters[];
};

layout(std430,binding = 4) buffer Transformations
{
	mat4 models[];
};

struct IndirectDrawInfo
{
	unsigned int  count;
	unsigned int  instanceCount;
	unsigned int  firstIndex;
	unsigned int  baseVertex;
	unsigned int  baseInstance;
};

layout(std430,binding = 2) buffer IndirectDrawBuffer
{
	IndirectDrawInfo draw_info[];
};

shared uint written = 0;

void main()
{
	if(written != 0)
	{
	return;
	}
	atomicAdd(written,1);
	IndirectDrawInfo this_draw_info;
	this_draw_info.count = 128;
	this_draw_info.instanceCount = 1;
	this_draw_info.firstIndex = clusters[gl_WorkGroupID.x].meshid_numinstances_numclusters_padding.
	
}