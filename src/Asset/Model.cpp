//deprecated
/*

#include "Model.h"
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags
#include "error.h"
#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define AI_CONFIG_PP_RVC_FLAGS aiComponent_COLORS
#define AI_CONFIG_PP_CT_MAX_SMOOTHING_ANGLE 80

std::vector<MeshDataAny> load_meshes() {
    
}

void Model::processNode(void* nodeIN, const void* sceneIN, bool hastangents)
{
    aiNode* node = static_cast<aiNode*>(nodeIN);
    const aiScene* scene = static_cast<const aiScene*>(sceneIN);

    //todo process materials too

    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        this->meshes.push_back(processMesh(mesh, hastangents));
        //processMesh will finally turn this data into our class
    }
    // then do the same for each of its children
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene, hastangents);
    }
}

void Model::processScene(std::string directory, const void* sceneIN, bool hastangents)
{
    const aiScene* scene = static_cast<const aiScene*>(sceneIN);

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
        this->meshes.push_back(processMesh(scene->mMeshes[i],hastangents));
    for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
        this->materials.push_back(processMaterial(scene->mMaterials[i],""));
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
        this->mesh_index_material_index[i] = scene->mMeshes[i]->mMaterialIndex;

    //todo same for materials

}

Mesh* Model::processMesh(void* meshIN, bool hastangents)
{
    aiMesh* aimesh = static_cast<aiMesh*>(meshIN);
    Mesh* mesh;
    if (hastangents)
        mesh = new Mesh3();
        //mesh = new Mesh4()            //todo complete when tangent model implemented
    else
        mesh = new Mesh3();

    std::vector<float> vertexDataInterleaved;
    vertexDataInterleaved.reserve(aimesh->mNumVertices * (8+ 3*hastangents));
    for (unsigned int i = 0;i < aimesh->mNumVertices;++i)
    {
        vertexDataInterleaved.push_back(aimesh->mVertices[i].x);
        vertexDataInterleaved.push_back(aimesh->mVertices[i].y);
        vertexDataInterleaved.push_back(aimesh->mVertices[i].z);

        vertexDataInterleaved.push_back(aimesh->mNormals[i].x);
        vertexDataInterleaved.push_back(aimesh->mNormals[i].y);
        vertexDataInterleaved.push_back(aimesh->mNormals[i].z);

        if (aimesh->mNumUVComponents[0] != 2)
        {
            vertexDataInterleaved.push_back(0.0f);
            vertexDataInterleaved.push_back(0.0f);
        }
        else
        {
            vertexDataInterleaved.push_back(aimesh->mTextureCoords[0][i].x);
            vertexDataInterleaved.push_back(aimesh->mTextureCoords[0][i].y);
        }

        if (hastangents)
        {
            vertexDataInterleaved.push_back(aimesh->mTangents[i].x);
            vertexDataInterleaved.push_back(aimesh->mTangents[i].y);
            vertexDataInterleaved.push_back(aimesh->mTangents[i].z);
        }
    }

    std::vector<unsigned int> index_data;
    index_data.reserve(aimesh->mNumFaces * 3);
    for (unsigned int i = 0; i < aimesh->mNumFaces; i++)
    {
        aiFace face = aimesh->mFaces[i];
        //assimp stores meshes as an array of faces and each face data structure has the order in which its vertices need to be drawn
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            index_data.push_back(face.mIndices[j]);
    }

    mesh->setupMesh(&vertexDataInterleaved[0],aimesh->mNumVertices * (sizeof(Vertex3)/sizeof(float)),index_data.data(),index_data.size());
    return mesh;
}

Material* Model::processMaterial(void* assimp_aiMaterial, std::string directory)
{
    struct tex_creation_info
    {
        int width; int height;
        unsigned char* data;
    };  //0 -> albedo+alpha ;; 1 -> roughness + metalness + ambient occlusion ;; 2-> normal map

    aiMaterial* mat = static_cast<aiMaterial*>(assimp_aiMaterial);


    tex_creation_info diffuse_alpha_data;
    tex_creation_info roughness_metallic_ao_data;

    constexpr int num_loads = 6;
    aiTextureType to_load[num_loads] = { aiTextureType_DIFFUSE_ROUGHNESS,aiTextureType_OPACITY,aiTextureType_METALNESS,aiTextureType_AMBIENT_OCCLUSION,aiTextureType_NORMALS , aiTextureType_EMISSIVE};
    struct tex_create_info
    {
        int w; int h; int channels;
        unsigned char* data = NULL;
        inline Texture_2D create_tex_2D(GLenum format,GLenum internal_format)
        {
            return Texture_2D(w, h, format, GL_UNSIGNED_BYTE, data, internal_format);
        }
    };
    tex_create_info load_dest[num_loads];
    tex_create_info roughness_dest;
    
    for (int i = 0; i < num_loads; ++i)
    {
        if (mat->GetTextureCount(to_load[i]) < 1)
            continue;
        aiString path;
        mat->GetTexture(to_load[i], 0, &path);
        std::string fullpath = directory.c_str();
        fullpath += path.C_Str();
        load_dest[i].data = stbi_load(fullpath.c_str(), &load_dest[i].w, &load_dest[i].h, &load_dest[i].channels, 0);
    }

    tex_create_info diffuse_alpha;

    //have to split roughness and diffuse cause amazing and nothing bad everything fine
    if (load_dest[0].data == NULL)
    {
        load_dest[0].w = 1;
        load_dest[0].h = 1;
        load_dest[0].channels = 3;
        aiColor3D diff_color;
        mat->Get(AI_MATKEY_COLOR_DIFFUSE, diff_color);
        load_dest[0].data = static_cast<unsigned char*>(calloc(3, sizeof(unsigned char)));
        load_dest[0].data[0] = 255 * diff_color.r;
        load_dest[0].data[1] = 255 * diff_color.g;
        load_dest[0].data[2] = 255 * diff_color.b;

        roughness_dest.w = 1;
        roughness_dest.h = 1;
        roughness_dest.channels = 1;
        roughness_dest.data = static_cast<unsigned char*>(calloc(1, sizeof(unsigned char)));
        roughness_dest.data[0] = 255 * 0.5f;
    }
    else
    {
        unsigned char* diffuse_only = static_cast<unsigned char*>(calloc((load_dest[0].w * load_dest[0].h * 3),sizeof(unsigned char)));
        unsigned char* roughness_only = static_cast<unsigned char*>(calloc((load_dest[0].w * load_dest[0].h * 1), sizeof(unsigned char)));
        
        size_t channel_index_3 = 0;
        size_t channel_index_1 = 0;

        for (size_t i = 0; i < load_dest[0].w * load_dest[0].h * 4; i += 4)
        {
            diffuse_only[channel_index_3] = load_dest[0].data[i];
            diffuse_only[channel_index_3+1] = load_dest[0].data[i+1];
            diffuse_only[channel_index_3+2] = load_dest[0].data[i+2];
            roughness_only[channel_index_1] = load_dest[0].data[i + 3];

            channel_index_3 += 3;
            channel_index_1 += 1;
        }
        stbi_image_free(load_dest[0].data);

        load_dest[0].data = diffuse_only;
        load_dest[0].channels = 3;

        roughness_dest.w = load_dest[0].w;
        roughness_dest.h = load_dest[0].h;
        roughness_dest.data = roughness_only;
        roughness_dest.channels = 1;
    }
    
    if (load_dest[1].data == NULL)
    {
        load_dest[1].w = 1;
        load_dest[1].h = 1;
        load_dest[1].channels = 1;
        float opacity;
        mat->Get(AI_MATKEY_OPACITY, opacity);
        load_dest[1].data = static_cast<unsigned char*>(calloc(1, sizeof(unsigned char)));
        load_dest[1].data[0] = 255 * opacity;
    }

    if (load_dest[2].data == NULL)
    {
        load_dest[2].w = 1;
        load_dest[2].h = 1;
        load_dest[2].channels = 1;
        aiColor3D specular;
        mat->Get(AI_MATKEY_COLOR_SPECULAR, specular);
        load_dest[2].data = static_cast<unsigned char*>(calloc(1, sizeof(unsigned char)));
        load_dest[2].data[0] = 255 * (specular.r + specular.g + specular.b) * 0.33f;

    }
    
    if (load_dest[3].data == NULL)
    {
        load_dest[3].w = 1;
        load_dest[3].h = 1;
        load_dest[3].channels = 1;
        load_dest[3].data = static_cast<unsigned char*>(calloc(1, sizeof(unsigned char)));
        load_dest[3].data[0] = 255;
    }

    if (load_dest[4].data == NULL)
    {
        load_dest[4].w = 1;
        load_dest[4].h = 1;
        load_dest[4].channels = 3;
        load_dest[4].data = static_cast<unsigned char*>(calloc(3, sizeof(unsigned char)));
        load_dest[4].data[0] = 0;
        load_dest[4].data[1] = 0;
        load_dest[4].data[2] = 255; //blue normal map, no bump
    }

    if (load_dest[5].data == NULL)
    {
        load_dest[5].w = 1;
        load_dest[5].h = 1;
        load_dest[5].channels = 1;
        float emmision;
        load_dest[5].data = static_cast<unsigned char*>(calloc(1, sizeof(unsigned char)));
        load_dest[5].data[0] = 0;   //no emmision
    }

    Material* this_material_beauty = new Material(
        load_dest[0].create_tex_2D(GL_RGB, GL_RGB),     //albedo
        roughness_dest.create_tex_2D(GL_RED, GL_RED),   //roughness
        load_dest[2].create_tex_2D(GL_RED,GL_RED),      //metalness
        load_dest[1].create_tex_2D(GL_RED,GL_RED),      //alpha
        load_dest[3].create_tex_2D(GL_RED,GL_RED),      //ambient occlusion
        load_dest[5].create_tex_2D(GL_RED,GL_RED),      //emission
        load_dest[4].create_tex_2D(GL_RGB,GL_RGB)       //normal
    );

    for (int i = 0; i < num_loads; ++i)
    {
        free(load_dest[i].data);
    }
    free(roughness_dest.data);

    return this_material_beauty;
}

void Model::loadModel(const char* filedirIN, bool loadTangents, bool optimizeMesh, bool smoothNormals)
{
    std::string model_loc = filedirIN;
    short last_dot = model_loc.find_last_of('.');
    if (model_loc.substr(last_dot, model_loc.size() - last_dot) == ".twomodel")
    {
        loadModel_int_format(filedirIN, loadTangents, optimizeMesh, smoothNormals);
    }
    else
    {
        loadModel_ext_format(filedirIN, loadTangents, optimizeMesh, smoothNormals);
    }
}

void Model::loadModel_int_format(const char* filedirIN, bool loadTangents, bool optimizeMesh, bool smoothNormals)
{
    BinaryFileUtility file(filedirIN);
    int num_meshes = 0;
    file.read(&num_meshes);
    meshes.reserve(num_meshes);
    for (int i = 0; i < num_meshes; i++)
    {
        uint32_t indices_size = 0;
        file.read(&indices_size);
        unsigned int* indices_data = new unsigned int[indices_size];
        file.read(indices_data, indices_size);
        uint32_t verts_size_float = 0;
        file.read(&verts_size_float);
        float* data = new float[verts_size_float];
        file.read(data, verts_size_float);
        Mesh3* mesh_ = new Mesh3;
        mesh_->setupMesh(data, verts_size_float,indices_data,indices_size);
        meshes.push_back(mesh_);
        
        delete[] indices_data;
        delete[] data;
    }
    file.close();
}

void Model::loadModel_ext_format(const char* filedirIN,bool loadTangents, bool optimizeMesh, bool smoothNormals)
{
	Assimp::Importer importer;
    aiPostProcessSteps normalStep;
    aiPostProcessSteps tangentStep;
    aiPostProcessSteps meshOptimizeStep;

    if (smoothNormals)
        normalStep = aiProcess_GenSmoothNormals;
    else
        normalStep = aiProcess_GenNormals;
    if (loadTangents)
        tangentStep = aiProcess_CalcTangentSpace;
    else
        tangentStep = static_cast<aiPostProcessSteps>(NULL); //todo seems wrong, please test
    if (optimizeMesh)
        meshOptimizeStep = static_cast<aiPostProcessSteps>(aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph); //todo seems wrong, please test
    else
        meshOptimizeStep = static_cast<aiPostProcessSteps>(NULL); //todo seems wrong, please test

    const aiScene* scene = importer.ReadFile(filedirIN,
        aiProcess_RemoveComponent |
        aiProcess_SortByPType |
        aiProcess_GenUVCoords |
        aiProcess_FlipUVs |
        normalStep |
        tangentStep |
        meshOptimizeStep |
        aiProcess_FindInvalidData |
        aiProcess_JoinIdenticalVertices |
        aiProcess_Triangulate
    );

    if (!scene)
    {
        std::string error = importer.GetErrorString();
        ENGINE2_THROW_ERROR(error.c_str());
        return;
    }

    this->meshes.reserve(scene->mNumMeshes);

    std::string directory;
    std::filesystem::path og_file_dir(filedirIN);


    processScene(og_file_dir.remove_filename().string(),scene, loadTangents);

}

*/