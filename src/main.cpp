#include "glad/glad.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include <memory>
#include <string>
#include <unordered_map>

#include <chrono>
#include <filesystem>
#include <functional>
#include <iostream>
#include <vector>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/norm.hpp"

#include "Graphics/Graphics_2.h"
#include "Graphics/HigherGraphics_2.h"

#include "Asset/Model.h"
#include "Asset/TextureAsset.h"
#include "Mesh/Mesh.h"

#include "Graphics/RenderPass.h"

#include "entt/entt.hpp"

std::shared_ptr<MeshStatic> mesh_static_loader(std::string filedir) {
  assert(filedir.substr(filedir.find_last_of('.'), filedir.size() - 1) ==
         ".twomesh");

  std::ifstream file(filedir, std::ios::binary);
  if (file.is_open() == false) {
    // todo error
  }
  char signature[] = "twomesh";
  file.read(signature, strlen(signature));
  unsigned short version[2] = {0, 0};
  file.read((char *)version, sizeof(unsigned short) * 2);
  assert(std::string(signature) == "TWOMESH" && version[0] == 1);

  unsigned int num_vertices = 0;
  file.read((char *)&num_vertices, sizeof(unsigned int));
  unsigned int num_triangles = 0;
  file.read((char *)&num_triangles, sizeof(unsigned int));

  std::vector<Vertex3> vertices;
  vertices.resize(num_vertices);
  file.read((char *)&vertices[0], vertices.size() * sizeof(Vertex3));

  std::vector<unsigned int> indices;
  indices.resize(static_cast<size_t>(num_triangles) * 3);
  file.read((char *)indices.data(), indices.size() * sizeof(unsigned int));

  file.close();
  return std::make_shared<MeshStatic>(MeshData(vertices, indices));
}

void mesh_static_unloader(std::shared_ptr<MeshStatic> ptr) { ptr->unload(); }

typedef Asset<MeshStatic, mesh_static_loader, mesh_static_unloader>
    MeshStaticAsset;

glm::mat4 compute_dir_lightspace(const glm::mat4 &proj, const glm::mat4 &view,
                                 const glm::vec3 &lightdir) {
  glm::vec4 left(std::numeric_limits<float>::max()),
      right(std::numeric_limits<float>::min()),
      bottom(std::numeric_limits<float>::max()),
      top(std::numeric_limits<float>::min()),
      near(std::numeric_limits<float>::max()),
      far(std::numeric_limits<float>::min());
  auto inv_view = glm::inverse(proj * view);
  glm::vec4 unitCube[2][2][2];
  for (int x = 0; x < 2; ++x)
    for (int y = 0; y < 2; ++y)
      for (int z = 0; z < 2; ++z) {
        unitCube[x][y][z] =
            glm::vec4(glm::vec3((float)x, (float)y, (float)z) * 2.f - 1.f, 1.0);
        unitCube[x][y][z] = inv_view * unitCube[x][y][z];
        unitCube[x][y][z] /= unitCube[x][y][z].w;
        // std::cout << unitCube[x][y][z].x << ", " << unitCube[x][y][z].y << ",
        // " << unitCube[x][y][z].z << std::endl;
      }
  for (int x = 0; x < 2; ++x)
    for (int y = 0; y < 2; ++y)
      for (int z = 0; z < 2; ++z) {
        auto c = unitCube[x][y][z];
        right = c.x > right.x ? c : right;
        left = c.x < left.x ? c : left;
        top = c.y > top.y ? c : top;
        bottom = c.y < bottom.y ? c : bottom;
        near = c.z < near.z ? c : near;
        far = c.z > far.z ? c : far;
      }
  // return glm::ortho(left.x, right.x, bottom.y, top.y, near.z, far.z);
  return proj * view;
}

void printBlockLimits() {
  int maxBlocks;
  glGetIntegerv(GL_MAX_VERTEX_UNIFORM_BLOCKS, &maxBlocks);
  std::cout << "\nMax vertex uniform blocks: " << maxBlocks;
  glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS, &maxBlocks);
  std::cout << "\nMax fragment uniform blocks: " << maxBlocks;
  glGetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, &maxBlocks);
  std::cout << "\nMax vertex shader storage blocks: " << maxBlocks;
  glGetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, &maxBlocks);
  std::cout << "\nMax fragment shader storage blocks: " << maxBlocks;
  glGetIntegerv(GL_MAX_COMPUTE_UNIFORM_BLOCKS, &maxBlocks);
  std::cout << "\nMax compute uniform blocks: " << maxBlocks;
  glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, &maxBlocks);
  std::cout << "\nMax compute shader storage blocks: " << maxBlocks;
}

int main() {

  std::cout << std::filesystem::current_path();
  GLFWwindow *window =
      Graphics::InitiateGraphicsLib({"GL_ARB_bindless_texture"});
  static int width, height;
  glfwGetWindowSize(window, &width, &height);

  glEnable(GL_DEPTH_TEST);
  glViewport(0, 0, width, height);
  glDepthFunc(GL_LESS);

  CamHandler cam(window);

  auto default_fbo = Framebuffer::default_framebuffer(width, height);

  typedef FramebufferAttachment FBOattach;

  // Graphics resources
  struct {
    Pipeline shadow_mapper = Pipeline(R"(src/Graphics/Shaders/Shadow.vert)",
                                      R"(src/Graphics/Shaders/Shadow.frag)");

  } pipes;
  struct {
    Buffer proj_view;
    // Buffer models;
    // Buffer material_ids;

    // Buffer materials;
    Buffer g_buffer_handles;
    Buffer active_material_id;
  } buffs;
  struct {

    Texture_2D shadow_map = Texture_2D(width, height, GL_DEPTH_COMPONENT32);
  } textures;
  struct {
  } renderbuffs;

  entt::registry registry;
  Framebuffer shadow_pass_fbo(width, height, {},
                              FBOattach(textures.shadow_map, true),
                              DepthStencilAttachType::ONLY_DEPTH);

  float near_far[2] = {0.001f, 100.0f};
  glm::mat4 projection_mat =
      glm::perspective(45.f, (float)width / height, near_far[0], near_far[1]);
  // constexpr int num_demo_meshes = 2;
  auto sphere_model = loadGLTF("3DModelData/brick sphere/bricksphere.gltf");
  std::vector<MaterialEntity> material_ids;
  for (MaterialPBR mat : sphere_model->materials) {
    auto entity = registry.create();
    registry.emplace<LoadedMaterialPBR>(entity, mat);
    registry.get<LoadedMaterialPBR>(entity).activate();
    material_ids.push_back(entity);
  }

  for(auto& mesh : sphere_model->meshes){
    auto entity = registry.create();
    registry.emplace<MeshStatic>(entity, mesh.first);
    registry.emplace<MaterialEntity>(entity, material_ids[mesh.second]);
    registry.emplace<Position>(entity, glm::vec3(0.0, 1.0, 2.0));
  }



  // FURTHER INITIALIZATIONS

  // making Textures resident
  textures.shadow_map.make_handle_resident();

  // Buffers initialization
  buffs.proj_view = Buffer(sizeof(glm::mat4) * 2, NULL);
  buffs.proj_view.modify(glm::value_ptr(projection_mat), sizeof(glm::mat4), 0);


  // buffs.materials = Buffer(sizeof(glm::vec4) * materials.size(), NULL);
  // buffs.materials.modify(materials.data(), sizeof(glm::vec4) * materials.size(),
  //                        0);

  Buffer indirect_draw_buffer =
      MeshStatic::get_static_meshes_holder().get_indirect_draw_buffer();

  auto indirect_draw_buf = registry.create();
  registry.emplace<Buffer>(indirect_draw_buf, indirect_draw_buffer);
  auto proj_view_buf = registry.create();
  registry.emplace<Buffer>(proj_view_buf, buffs.proj_view);
  auto materials_buf = registry.create();
  registry.emplace<Buffer>(materials_buf);
  auto shadow_map_tex = registry.create();
  registry.emplace<Texture_2D>(shadow_map_tex, textures.shadow_map);
  auto active_material_id_buf = registry.create();
  registry.emplace<Buffer>(active_material_id_buf, buffs.active_material_id);
  auto g_buffer_handles_buf = registry.create();
  registry.emplace<Buffer>(g_buffer_handles_buf, buffs.g_buffer_handles);

  GeometryPass geometry_pass(width, height, indirect_draw_buf, proj_view_buf);
  geometry_pass.init(registry);
  std::array<uint64_t, 2> gbuff_handles = {
      registry.get<Texture_2D>(geometry_pass.g_pos_tex).get_handle(),
      registry.get<Texture_2D>(geometry_pass.g_norm_tex).get_handle()};
  registry.patch<Buffer>(
      g_buffer_handles_buf, [&gbuff_handles](Buffer &buffer) {
        buffer = Buffer(sizeof(uint64_t) * gbuff_handles.size(), NULL);
        buffer.modify(gbuff_handles.data(), sizeof(uint64_t) * 2, 0);
      });

  registry.get<Buffer>(active_material_id_buf) = Buffer(sizeof(uint32_t), NULL);


  MaterialPass material_pass(width, height,
                             active_material_id_buf);
  material_pass.init(registry);
  TextureToDepthPass texture_to_depth_pass(geometry_pass.material_depth_tex,
                                           material_pass.material_pass_fbo);
  texture_to_depth_pass.init(registry);

  LightingPass lighting_pass(geometry_pass.g_pos_tex, geometry_pass.g_norm_tex,
                             material_pass.material_pass_out_tex, width,
                             height);
  lighting_pass.init(registry);

  // for (int i = 0; i < demo_meshes.size(); ++i) {
  //   registry.get<Buffer>(models_buf)
  //       .modify(glm::value_ptr(demo_meshes[i].model), sizeof(glm::mat4),
  //                sizeof(glm::mat4) * i);
  //   uint32_t actual_mat_id = demo_meshes[i].mat_id + 1;
  //   registry.get<Buffer>(material_ids_buf)
  //       .modify(&actual_mat_id, sizeof(uint32_t), sizeof(uint32_t) * i);
  // }

  // PIPELINES SETUP

  pipes.shadow_mapper.add_pipeline_uniform_block("Matrices");
  pipes.shadow_mapper.bind_pipeline_uniform_block("Matrices", 1);
  pipes.shadow_mapper.add_pipeline_ssbo_block("Transformations");
  pipes.shadow_mapper.bind_pipeline_ssbo_block("Transformations", 2);

  // binding buffers to shader blocks
  auto geometry = &registry.get<Pipeline>(geometry_pass.geometry_pipe);
  buffs.proj_view.bind_base(
      GL_UNIFORM_BUFFER,
      geometry->get_pipeline_uniform_block_binding("Matrices"));
  // buffs.models.bind_base(
  //     GL_SHADER_STORAGE_BUFFER,
  //     geometry->get_pipeline_ssbo_block_binding("Transformations"));
  // buffs.material_ids.bind_base(
  //     GL_SHADER_STORAGE_BUFFER,
  //     geometry->get_pipeline_ssbo_block_binding("MaterialIds"));

  double delta_time = 0.0;

  glBindTexture(GL_TEXTURE_2D, 0);

  Graphics::clear_color(0.3, 0.3, 0.3, 1);

  DebugTextureDrawer tex_drawer;
  tex_drawer.set_uniform_vec<float, 2>("near_far", &near_far[0]);
  // todo make better ways
  glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glm::vec3 LIGHTPOS = glm::vec3(0.5, 1, 1) * 10.f;
  glm::mat4 LIGHTVIEW = glm::lookAt(LIGHTPOS, glm::vec3(0), glm::vec3(0, 1, 0));
  // pipes.material.set_uniform_vec<3,float>(pipes.material.get_uniform_loc("lightPos"),
  // (float*)&LIGHTPOS);
  auto lightspacetrans_entity = registry.create();
  registry.emplace<glm::mat4>(lightspacetrans_entity, glm::mat4(1));

  while (!glfwWindowShouldClose(window) &&
         !(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)) {
    double start_time = glfwGetTime();
    // update view matrix
    cam.moveAround(delta_time);
    cam.lookAround();
    glm::vec3 campos = cam.get_pos();
    // pipes.material.set_uniform_vec<3>(pipes.material.get_uniform_loc("camera_pos"),
    // glm::value_ptr(campos));
    glm::mat4 curr_view = cam.getView();

    shadow_pass_fbo.bind(GL_FRAMEBUFFER);
    shadow_pass_fbo.clear(GL_DEPTH_BUFFER_BIT);
    pipes.shadow_mapper.bind();
    // todo do in better ways
    buffs.proj_view.modify(glm::value_ptr(LIGHTVIEW), sizeof(glm::mat4),
                           sizeof(glm::mat4));
    registry.get<glm::mat4>(lightspacetrans_entity) =
        compute_dir_lightspace(projection_mat, curr_view, LIGHTPOS);
    pipes.shadow_mapper.set_uniform_mat<4, 4>(
        pipes.shadow_mapper.get_uniform_loc("lightspace"),
        glm::value_ptr(registry.get<glm::mat4>(lightspacetrans_entity)));

    registry.get<Buffer>(indirect_draw_buf).bind(GL_DRAW_INDIRECT_BUFFER);
    MeshStatic::get_static_meshes_holder().multi_draw();

    buffs.proj_view.modify(glm::value_ptr(curr_view), sizeof(glm::mat4),
                           sizeof(glm::mat4));

    geometry_pass.execute(registry);

    texture_to_depth_pass.execute(registry);

    material_pass.execute(registry);

    lighting_pass.execute(registry);

    default_fbo.bind(GL_FRAMEBUFFER);
    default_fbo.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    tex_drawer(
        registry.get<Texture_2D>(lighting_pass.lighting_pass_out_tex));

    glfwSwapBuffers(window);
    glfwPollEvents();
    delta_time = glfwGetTime() - start_time;
  }

  return 0;
}
