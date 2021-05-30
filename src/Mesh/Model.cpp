#include "Model.h"
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags
#define AI_CONFIG_PP_RVC_FLAGS aiComponent_COLORS
#define AI_CONFIG_PP_CT_MAX_SMOOTHING_ANGLE 80

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

void Model::processScene(const void* sceneIN, bool hastangents)
{
    const aiScene* scene = static_cast<const aiScene*>(sceneIN);

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
        this->meshes.push_back(processMesh(scene->mMeshes[i],hastangents));
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

        vertexDataInterleaved.push_back(aimesh->mTextureCoords[0][i].x);
        vertexDataInterleaved.push_back(aimesh->mTextureCoords[0][i].y);

        if (hastangents)
        {
            vertexDataInterleaved.push_back(aimesh->mTangents[i].x);
            vertexDataInterleaved.push_back(aimesh->mTangents[i].y);
            vertexDataInterleaved.push_back(aimesh->mTangents[i].z);
        }
    }

    mesh->setupMesh(&vertexDataInterleaved[0], vertexDataInterleaved.size());

    return mesh;
}

void Model::loadModel(const char* filedirIN,bool loadTangents, bool optimizeMesh, bool smoothNormals)
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
        aiProcess_CalcTangentSpace |
        aiProcess_Triangulate |
        aiProcess_SortByPType |
        aiProcess_GenUVCoords |
        aiProcess_FlipUVs |
        normalStep |
        tangentStep |
        meshOptimizeStep |
        aiProcess_FindInvalidData |
        aiProcess_JoinIdenticalVertices
    );

    if (!scene)
    {
        //todo errorlog
        std::string error = importer.GetErrorString();
        return;
    }

    this->meshes.reserve(scene->mNumMeshes);
    processScene(scene, loadTangents);

}