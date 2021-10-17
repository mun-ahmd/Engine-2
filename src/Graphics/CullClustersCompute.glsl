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
struct ClusterInfo
{
	unsigned int mesh_id;
	unsigned int num_triangles;
};
struct Cluster	//no padding required
{
	Triangle triangles[128];
};
struct MeshInformation
{
	unsigned int num_instances;
	unsigned int base_vertex;
};
struct IndirectDrawInfo
{
	unsigned int  count;
	unsigned int  instanceCount;
	unsigned int  firstIndex;
	unsigned int  baseVertex;
	unsigned int  baseInstance;
};


layout(std430, binding = 4) buffer ClustersInfo
{
	ClusterInfo cluster_info[];
};
layout(std430, binding = 2) buffer StaticMeshData
{
	Cluster clusters[];
};
layout(std430,binding = 5) buffer MeshInfo
{
//this is not part of ClusterInfo because this can change a lot and modifying the cluster info would be performance damaging
	MeshInformation mesh_info[];
};
layout(std430,binding = 1) buffer Transformations
{
	mat4 models[];
};
layout(std430,binding = 3) coherent buffer IndirectDrawBuffer
{
	IndirectDrawInfo draw_info[];
};


//shared uint written = 0;

void main()
{
//	if(written != 0)
//	{
//	return;
//	}
//	
//	atomicAdd(written,1);
	IndirectDrawInfo this_draw_info;
	this_draw_info.count = 384;
	this_draw_info.instanceCount = 1;
	this_draw_info.firstIndex = gl_WorkGroupID.x*384;
	this_draw_info.baseVertex = mesh_info[cluster_info[gl_WorkGroupID.x].mesh_id].base_vertex;
	this_draw_info.baseInstance = cluster_info[gl_WorkGroupID.x].mesh_id;
	draw_info[gl_WorkGroupID.x] = this_draw_info;
}