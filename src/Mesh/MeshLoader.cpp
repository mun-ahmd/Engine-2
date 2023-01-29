#include "Graphics/Graphics_2.h"
#include "Mesh.h"
#include "glad/glad.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <numeric>
#include <utility>
#include <vector>
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "glm/glm.hpp"
#include "json.hpp"
#include "stb_image.h"
#include "stb_image_write.h"
#include <algorithm>
#include <any>
#include <filesystem>
#include <optional>
#define TINYGLTF_NO_INCLUDE_JSON
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#define TINYGLTF_USE_CPP14
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include "tiny_gltf.h"

using namespace tinygltf;
using json = nlohmann::json;

struct AccessorInfo {
  Accessor accessor;
  BufferView bufferView;
  tinygltf::Buffer buffer;
};

AccessorInfo loadAccessor(Model &model, int accessor_index) {
  AccessorInfo info;
  info.accessor = model.accessors[accessor_index];
  info.bufferView = model.bufferViews[info.accessor.bufferView];
  info.buffer = model.buffers[info.bufferView.buffer];
  return info;
}

std::vector<std::pair<MeshData<Vertex3>, int>> loadMeshes(Model &model) {
  std::vector<std::pair<MeshData<Vertex3>, int>> meshes;
  size_t num_total_primitives = 0;
  for (const auto &mesh : model.meshes)
    num_total_primitives += mesh.primitives.size();
  meshes.reserve(num_total_primitives);
  std::array<std::string, 3> required = {"POSITION", "NORMAL", "TEXCOORD_0"};
  std::array<AccessorInfo, 3> accessorsInfo;
  for (auto &mesh : model.meshes) {
    for (auto &primitive : mesh.primitives) {
      MeshData<Vertex3> data;
      // todo make sure primitive mode = 4 (TRIANGLES)
      if (primitive.mode != 4) {
        //! panic
        continue;
      }
      for (int i = 0; i < 3; ++i) {
        if (primitive.attributes.find(required[i]) ==
            primitive.attributes.end()) {
          //! panic
          continue;
        }
        accessorsInfo[i] =
            loadAccessor(model, primitive.attributes[required[i]]);
        if (accessorsInfo[i].accessor.type != TINYGLTF_COMPONENT_TYPE_FLOAT) {
          //! panic
          continue;
        }
      }
      unsigned char *position = accessorsInfo[0].buffer.data.data() +
                                accessorsInfo[0].bufferView.byteOffset +
                                accessorsInfo[0].accessor.byteOffset;
      int position_stride = accessorsInfo[0].bufferView.byteStride == 0
                                ? sizeof(glm::vec3)
                                : accessorsInfo[0].bufferView.byteStride;

      unsigned char *normal = accessorsInfo[1].buffer.data.data() +
                              accessorsInfo[1].bufferView.byteOffset +
                              accessorsInfo[1].accessor.byteOffset;
      int normal_stride = accessorsInfo[1].bufferView.byteStride == 0
                              ? sizeof(glm::vec3)
                              : accessorsInfo[1].bufferView.byteStride;

      unsigned char *uv = accessorsInfo[2].buffer.data.data() +
                          accessorsInfo[2].bufferView.byteOffset +
                          accessorsInfo[2].accessor.byteOffset;
      int uv_stride = accessorsInfo[2].bufferView.byteStride == 0
                          ? sizeof(glm::vec2)
                          : accessorsInfo[2].bufferView.byteStride;

      data.vertices.reserve(accessorsInfo.front().accessor.count);
      for (int i = 0; i < accessorsInfo.front().accessor.count; ++i) {
        Vertex3 curr;
        memcpy(&curr.pos, position, sizeof(glm::vec3));
        position += position_stride;
        memcpy(&curr.norm, normal, sizeof(glm::vec3));
        normal += normal_stride;
        memcpy(&curr.uv, uv, sizeof(glm::vec2));
        uv += uv_stride;
        data.vertices.push_back(curr);
      }
      auto indicesAccessorInfo = loadAccessor(model, primitive.indices);
      if (indicesAccessorInfo.accessor.componentType ==
          TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
        auto bufferIterator = indicesAccessorInfo.buffer.data.begin() +
                              indicesAccessorInfo.bufferView.byteOffset +
                              indicesAccessorInfo.accessor.byteOffset;
        data.indices = std::vector<unsigned int>(
            bufferIterator,
            bufferIterator + indicesAccessorInfo.accessor.count);
      } else if (indicesAccessorInfo.accessor.componentType ==
                 TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        unsigned short *buffer = reinterpret_cast<unsigned short *>(
            indicesAccessorInfo.buffer.data.data() +
            indicesAccessorInfo.accessor.byteOffset +
            indicesAccessorInfo.bufferView.byteOffset);
        data.indices = std::vector<unsigned int>(
            buffer, buffer + indicesAccessorInfo.accessor.count);
      } else if (indicesAccessorInfo.accessor.componentType ==
                 TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
        unsigned int *buffer = reinterpret_cast<unsigned int *>(
            indicesAccessorInfo.buffer.data.data() +
            indicesAccessorInfo.accessor.byteOffset +
            indicesAccessorInfo.bufferView.byteOffset);
        data.indices = std::vector<unsigned int>(
            buffer, buffer + indicesAccessorInfo.accessor.count);
      } else {
        // invalid component type for indices !panic
      }
      meshes.push_back(std::make_pair(std::move(data), primitive.material));
    }
  }
  return meshes;
}

std::vector<MaterialPBR> loadMaterials(std::string gltfFile) {
  std::vector<MaterialPBR> materials;
  std::ifstream file(gltfFile);
  json gltf = json::parse(file);
  json materials_gltf = gltf["materials"];
  std::string materialname = gltf["materials"][0]["name"].get<std::string>();
  for (auto mat = materials_gltf.begin(); mat != materials_gltf.end(); mat++) {
    MaterialPBR curr;
    if (mat->contains("pbrMetallicRoughness")) {
      auto currMetallicRoughness = (*mat)["pbrMetallicRoughness"];
      if (currMetallicRoughness.contains("baseColorTexture")) {
        json baseColorTexture =
            (gltf["textures"][currMetallicRoughness["baseColorTexture"]["index"]
                                  .get<int>()]);
        curr.baseColorTex =
            std::filesystem::path(gltfFile).parent_path().string() +
            std::filesystem::path::preferred_separator +
            std::filesystem::path(gltf["images"][baseColorTexture["source"].get<int>()]["uri"]
                .get<std::string>()).string();
      }
      if (currMetallicRoughness.contains("metallicRoughnessTexture")) {
        json metallicRoughnessTexture =
            (gltf["textures"]
                 [currMetallicRoughness["metallicRoughnessTexture"]["index"]
                      .get<int>()]);
        curr.metallicRoughnessTex =
            std::filesystem::path(gltfFile).parent_path().string() +
            std::filesystem::path::preferred_separator +
            std::filesystem::path(gltf["images"][metallicRoughnessTexture["source"].get<int>()]["uri"]
                .get<std::string>()).string();
      }
      if (currMetallicRoughness.contains("baseColorFactor")) {
        auto baseColor =
            currMetallicRoughness["baseColorFactor"].get<std::vector<float>>();
        memcpy(&curr.baseColorFactor, baseColor.data(),
               sizeof(curr.baseColorFactor));
      }
      if (currMetallicRoughness["metallicFactor"].is_null() == false) {
        curr.metallic_factor =
            currMetallicRoughness["metallicFactor"].get<float>();
      }
      if (currMetallicRoughness.contains("roughnessFactor")) {
        curr.roughness_factor =
            currMetallicRoughness["roughnessFactor"].get<float>();
      }
    }

    if (mat->contains("normalTexture")) {
      json normalTexture =
          (gltf["textures"][(*mat)["normalTexture"]["index"].get<int>()]);
      curr.normalMap =
          std::filesystem::path(gltfFile).parent_path().string() +
          std::filesystem::path::preferred_separator +
          std::filesystem::path(gltf["images"][normalTexture["source"].get<int>()]["uri"]
              .get<std::string>()).string();
          
      if ((*mat)["normalTexture"].contains("scale")) {
        curr.normalScale = (*mat)["normalTexture"]["scale"].get<float>();
      }
    }
    materials.push_back(curr);
  }
  std::cout << materials.front().baseColorTex << std::endl;
  return materials;
}

std::optional<ModelData> loadGLTF(const char *filepath) {
  Model model;
  TinyGLTF loader;
  std::string err;
  std::string warn;
  std::filesystem::path path(filepath);
  bool ret;
  if (path.extension() == ".gltf")
    ret = loader.LoadASCIIFromFile(&model, &err, &warn, filepath);
  else if (path.extension() == ".glb")
    ret = loader.LoadBinaryFromFile(&model, &err, &warn,
                                    filepath); // for binary glTF(.glb)
  else
    ret = false;
  if (!warn.empty()) {
    std::cerr << "Warn: " << warn.c_str() << "\n";
  }

  if (!err.empty()) {
    std::cerr << "Err: " << err.c_str() << "\n";
    return std::nullopt;
  }

  if (!ret) {
    std::cerr << "glTF: Failed to parse file: " << filepath << " \n";
    return std::nullopt;
    
  }

  std::vector<std::pair<glm::mat4, int>> instances;
  instances.reserve(model.nodes.size());
  for (auto &node : model.nodes) {
    if (node.mesh != -1) {
      glm::mat4 curr_transform = glm::mat4(1.f);
      if (node.matrix.size() == 16) {
        curr_transform = glm::make_mat4(node.matrix.data());
      } else {
        glm::vec3 translation = glm::vec3(0.f);
        glm::quat rotation = glm::quat(glm::vec3(0.0));
        glm::vec3 scale = glm::vec3(1.f);

        if (node.translation.size() == 3)
          translation = glm::vec3(node.translation[0], -node.translation[1], node.translation[2]);
        if (node.rotation.size() == 4)
          rotation = glm::make_quat(node.rotation.data());
        if (node.scale.size() == 3)
          scale = glm::make_vec3(node.scale.data());

        curr_transform =
            glm::translate(curr_transform, translation) *
            glm::mat4_cast(rotation) *
            glm::scale(curr_transform, scale);
        // curr_transform = glm::mat4(1);
      }
      instances.push_back(std::make_pair(curr_transform, node.mesh));
    }
  }

  auto loaded = loadMeshes(model);
  ModelData modelData;
  modelData.meshes = std::move(loaded);
  modelData.materials = std::move(loadMaterials(filepath));
  modelData.instances = std::move(instances);
  return modelData;
}

// void modifyModelFormat(Model &model,
//                        const std::vector<MeshData<Vertex3>> &data) {
//    TODO COMPLETE THIS FUNCTION TO MODIFY GLTF FILE TO MAKE LOADING FASTER
//    AFTER INITIAL LOAD
//   int buf_index = model.buffers.size();
//   tinygltf::Buffer meshdata;
//   std::vector<tinygltf::BufferView> meshviews;
//   meshviews.reserve(data.size());
//   for (const auto &mesh : data)
//     meshdata.data.insert(
//         meshdata.data.end(),
//         reinterpret_cast<const unsigned char *>(mesh.vertices.data()),
//         reinterpret_cast<const unsigned char *>(mesh.vertices.data() +
//                                                 mesh.vertices.size()));

//   tinygltf::BufferView vertex_view;
//   vertex_view.buffer = buf_index;
//   vertex_view.byteLength = meshdata.data.size();
//   vertex_view.byteOffset = 0;
//   vertex_view.byteStride = sizeof(Vertex3);

//   int view_index = model.bufferViews.size();

//   tinygltf::Accessor pos_acc;
//   pos_acc.bufferView = view_index;
//   pos_acc.byteOffset = 0;
//   pos_acc.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
//   pos_acc.type = TINYGLTF_TYPE_VEC3;

//   tinygltf::Accessor norm_acc = pos_acc;
//   tinygltf::Accessor uv_acc = pos_acc;
//   uv_acc.type = TINYGLTF_TYPE_VEC2;

// }