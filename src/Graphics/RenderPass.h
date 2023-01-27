#pragma once
#include "Graphics/Graphics_2.h"
#include "HigherGraphics_2.h"
#include "Lights.h"
#include "Mesh/Mesh.h"
#include "entt/entity/fwd.hpp"
#include "entt/entt.hpp"
#include "glad/glad.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/fwd.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <algorithm>
#include <cstdint>
#include <vector>

class GeometryPass : public RenderPass {
  typedef FramebufferAttachment FBOattach;

private:
  entt::entity proj_view_buf;
  entt::entity indirect_draw_buf;

  int width;
  int height;

public:
  entt::entity visibility_tex;
  entt::entity g_buffer_fbo;
  entt::entity transformations_buf;
  entt::entity interpolate_tex;
  entt::entity material_depth_tex;
  entt::entity geometry_pipe;

  GeometryPass(int width, int height, entt::entity indirect_draw_buf,
               entt::entity proj_view_buf)
      : width(width), height(height), indirect_draw_buf(indirect_draw_buf),
        proj_view_buf(proj_view_buf) {}

  void init(entt::registry &registry) override {
    Texture_2D material_depth = Texture_2D(width, height, GL_RGB32F);
    material_depth.make_handle_resident();
    visibility_tex = registry.create();
    registry.emplace<Texture_2D>(visibility_tex,
                                 Texture_2D(width, height, GL_RG32UI));
    registry.get<Texture_2D>(visibility_tex).make_handle_resident();

    material_depth_tex = registry.create();
    registry.emplace<Texture_2D>(material_depth_tex, material_depth);
    Renderbuffer geometry_pass_depth =
        Renderbuffer(width, height, GL_DEPTH24_STENCIL8);

    interpolate_tex = registry.create();
    registry.emplace<Texture_2D>(interpolate_tex,
                                 Texture_2D(width, height, GL_RGB));

    Framebuffer g_buffer(
        width, height,
        {FBOattach(registry.get<Texture_2D>(visibility_tex), true),
         FBOattach(registry.get<Texture_2D>(interpolate_tex), true),
         FBOattach(material_depth, true)},
        FBOattach(geometry_pass_depth, false),
        DepthStencilAttachType::BOTH_DEPTH_STENCIL);
    g_buffer_fbo = registry.create();
    registry.emplace<Framebuffer>(g_buffer_fbo, g_buffer);

    auto meshes_view = registry.view<MeshStatic, Position, MaterialEntity>();
    std::vector<glm::mat4> transforms;
    transforms.reserve(meshes_view.size_hint());
    std::vector<uint32_t> material_ids;
    material_ids.reserve(meshes_view.size_hint());
    for (auto &entity : meshes_view) {
      transforms.push_back(
          glm::translate(glm::mat4(1.0), registry.get<Position>(entity)));
      // todo adding 1 to material id is inconvenient and possible future bug
      // causer but necessary to avoid 0s
      material_ids.push_back(
          static_cast<uint32_t>(registry.get<MaterialEntity>(entity)) + 1);
    }
    transformations_buf = registry.create();
    Buffer Transformations =
        Buffer(sizeof(glm::mat4) * transforms.size(), transforms.data());
    registry.emplace<Buffer>(transformations_buf, Transformations);

    Buffer MaterialIds =
        Buffer(sizeof(uint32_t) * material_ids.size(), material_ids.data());
    Transformations.bind_base(GL_SHADER_STORAGE_BUFFER, 2);
    MaterialIds.bind_base(GL_SHADER_STORAGE_BUFFER, 3);

    Pipeline geometry = Pipeline(R"(src/Graphics/Shaders/geometry.vert)",
                                 R"(src/Graphics/Shaders/geometry.frag)");
    geometry.add_pipeline_uniform_block("Matrices");
    geometry.bind_pipeline_uniform_block("Matrices", 1);
    geometry.add_pipeline_ssbo_block("Transformations");
    geometry.bind_pipeline_ssbo_block("Transformations", 2);
    geometry.add_pipeline_ssbo_block("MaterialIds");
    geometry.bind_pipeline_ssbo_block("MaterialIds", 3);
    geometry_pipe = registry.create();
    registry.emplace<Pipeline>(geometry_pipe, geometry);
  }
  void execute(entt::registry &registry) override {
    auto g_buffer = registry.get<Framebuffer>(g_buffer_fbo);
    auto geometry = registry.get<Pipeline>(geometry_pipe);
    auto indirect_draw_buffer = registry.get<Buffer>(indirect_draw_buf);
    g_buffer.bind(GL_FRAMEBUFFER);
    g_buffer.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GLuint clearcolor[4] = {0, 0, 0, 0};
    glClearBufferuiv(GL_COLOR, 0, clearcolor);
    geometry.bind();

    indirect_draw_buffer.bind(GL_DRAW_INDIRECT_BUFFER);
    MeshStatic::get_static_meshes_holder().multi_draw();
  }
};

class TextureToDepthPass : public RenderPass {
private:
  entt::entity material_depth_tex;
  entt::entity material_pass_fbo;

public:
  entt::entity tex_to_depth_pipe;

  TextureToDepthPass(entt::entity material_depth_tex,
                     entt::entity material_pass_fbo)
      : material_depth_tex(material_depth_tex),
        material_pass_fbo(material_pass_fbo) {}
  void init(entt::registry &registry) override {
    Pipeline tex_to_depth =
        Pipeline(R"(src/Graphics/Shaders/FullScreen.vert)",
                 R"(src/Graphics/Shaders/TextureToDepth.frag)");
    tex_to_depth.change_depth_func(GL_ALWAYS);
    tex_to_depth.set_uniform<uint64_t>(
        tex_to_depth.get_uniform_loc("depth_texture"),
        registry.get<Texture_2D>(material_depth_tex).get_handle());
    tex_to_depth_pipe = registry.create();
    registry.emplace<Pipeline>(tex_to_depth_pipe, tex_to_depth);
  }
  void execute(entt::registry &registry) override {
    auto tex_to_depth = &registry.get<Pipeline>(tex_to_depth_pipe);
    auto material_pass_framebuff =
        &registry.get<Framebuffer>(material_pass_fbo);
    material_pass_framebuff->bind(GL_FRAMEBUFFER);
    material_pass_framebuff->clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    tex_to_depth->bind();
    glDrawArrays(GL_TRIANGLES, 0, 3);
  }
};

class MaterialPass : public RenderPass {
  typedef FramebufferAttachment FBOattach;

private:
  entt::entity active_material_id_buf;
  entt::entity visibility_tex;
  entt::entity transformations_buf;
  entt::entity proj_view_buf;
  int width;
  int height;

public:
  entt::entity material_pipe;
  entt::entity material_pass_fbo;
  entt::entity material_pass_out_tex;
  entt::entity current_material_info_buffer;
  entt::entity g_pos_tex;
  entt::entity g_norm_tex;

  MaterialPass(int width, int height, entt::entity active_material_id_buf,
               entt::entity visibility_tex, entt::entity transformations_buf,
               entt::entity proj_view_buf)
      : width(width), height(height),
        active_material_id_buf(active_material_id_buf),
        visibility_tex(visibility_tex),
        transformations_buf(transformations_buf), proj_view_buf(proj_view_buf) {
  }

  void init(entt::registry &ecs) override {

    Texture_2D g_pos = Texture_2D(width, height, GL_RGB32F);
    g_pos.make_handle_resident();
    g_pos_tex = ecs.create();
    ecs.emplace<Texture_2D>(g_pos_tex, g_pos);
    Texture_2D g_norm = Texture_2D(width, height, GL_RGB32F);
    g_norm.make_handle_resident();
    g_norm_tex = ecs.create();
    ecs.emplace<Texture_2D>(g_norm_tex, g_norm);
    Pipeline material = Pipeline(R"(src/Graphics/Shaders/MaterialShader.vert)",
                                 R"(src/Graphics/Shaders/MaterialShader.frag)");
    material.change_depth_func(GL_EQUAL);

    material.add_pipeline_uniform_block("MaterialID");
    material.bind_pipeline_uniform_block("MaterialID", 2);
    material.add_pipeline_uniform_block("Materials");
    material.bind_pipeline_uniform_block("Materials", 4);
    material.add_pipeline_ssbo_block("IndirectDrawBuffer");
    material.bind_pipeline_ssbo_block("IndirectDrawBuffer", 10);
    material.add_pipeline_ssbo_block("IndicesData");
    material.bind_pipeline_ssbo_block("IndicesData", 11);
    material.add_pipeline_ssbo_block("VertexData");
    material.bind_pipeline_ssbo_block("VertexData", 12);
    material.add_pipeline_uniform_block("Matrices");
    material.bind_pipeline_uniform_block("Matrices", 1);
    material.add_pipeline_ssbo_block("Transformations");
    material.bind_pipeline_ssbo_block("Transformations", 2);

    MeshStatic::get_static_meshes_holder().indirect_draw_info.bind_base(
        GL_SHADER_STORAGE_BUFFER, 10);
    MeshStatic::get_static_meshes_holder().indices.bind_base(
        GL_SHADER_STORAGE_BUFFER, 11);
    MeshStatic::get_static_meshes_holder().vertices.bind_base(
        GL_SHADER_STORAGE_BUFFER, 12);

    ecs.get<Buffer>(active_material_id_buf)
        .bind_base(GL_UNIFORM_BUFFER,
                   material.get_pipeline_uniform_block_binding("MaterialID"));

    ecs.get<Texture_2D>(visibility_tex).make_handle_resident();
    material.set_uniform<uint64_t>(
        material.get_uniform_loc("visibility_tex"),
        ecs.get<Texture_2D>(visibility_tex).get_handle());

    glm::vec2 width_height = glm::vec2((float)width, (float)height);
    material.set_uniform_vec<2, float>(material.get_uniform_loc("width_height"),
                                       (float *)&width_height);

    material_pipe = ecs.create();
    ecs.emplace<Pipeline>(material_pipe, material);

    Texture_2D material_pass_out = Texture_2D(width, height, GL_RGBA);
    material_pass_out_tex = ecs.create();
    ecs.emplace<Texture_2D>(material_pass_out_tex, material_pass_out);
    Renderbuffer material_pass_depth =
        Renderbuffer(width, height, GL_DEPTH_COMPONENT32F);
    material_pass_fbo = ecs.create();
    ecs.emplace<Framebuffer>(
        material_pass_fbo,
        Framebuffer(width, height,
                    {FBOattach(material_pass_out, true), FBOattach(g_pos, true),
                     FBOattach(g_norm, true)},
                    FBOattach(material_pass_depth, false),
                    DepthStencilAttachType::ONLY_DEPTH));

    Buffer current_material_info(sizeof(LoadedMaterialPBR::BufferDataPBR),
                                 NULL);
    current_material_info.bind_base(
        GL_UNIFORM_BUFFER,
        material.get_pipeline_uniform_block_binding("Materials"));
    current_material_info_buffer = ecs.create();
    ecs.emplace<Buffer>(current_material_info_buffer, current_material_info);
  }

  void execute(entt::registry &ecs) override {
    auto current_material_info = &ecs.get<Buffer>(current_material_info_buffer);
    auto materials_view = ecs.view<LoadedMaterialPBR>();
    auto material = &ecs.get<Pipeline>(material_pipe);
    material->bind();
    ecs.get<Framebuffer>(material_pass_fbo).bind(GL_FRAMEBUFFER);

    uint32_t i = 0;
    for (auto entity : materials_view) {
      uint32_t material_id = i + 1;
      ecs.get<Buffer>(active_material_id_buf)
          .modify(&material_id, sizeof(uint32_t), 0);
      auto curr = materials_view.begin() + i;
      auto data = materials_view.get<LoadedMaterialPBR>(entity).getBufferData();
      current_material_info->modify(&data, sizeof(data), 0);
      glDrawArrays(GL_TRIANGLES, 0, 3);
      i += 1;
    }
  }
};

class LightingPass : public RenderPass {
  typedef FramebufferAttachment FBOattach;

private:
  entt::entity gpos_tex;
  entt::entity gnorm_tex;
  entt::entity albedo_tex;
  int width;
  int height;

public:
  entt::entity gbuffer_handles_buf;
  entt::entity nonshadow_point_lights_buf;
  entt::entity lighting_pass_pipe;
  entt::entity lighting_pass_fbo;
  entt::entity lighting_pass_out_tex;

  LightingPass(entt::entity gpos_tex, entt::entity gnorm_tex,
               entt::entity albedo_tex, int width, int height)
      : gpos_tex(gpos_tex), gnorm_tex(gnorm_tex), albedo_tex(albedo_tex),
        width(width), height(height) {}

  void init(entt::registry &ecs) override {
    for (int x = -3; x < 3; x++) {
      for (int y = -2; y < 2; y++) {
        auto entity = ecs.create();
        ecs.emplace<PointLight>(
            entity,
            PointLight(glm::vec3(3.0 * x, 4.0, 2.0 * y), 5.0,
                       glm::vec3(sin(glfwGetTime() * x), cos(glfwGetTime() * y),
                                 sin(glfwGetTime()) > cos(glfwGetTime())
                                     ? sin(glfwGetTime())
                                     : cos(glfwGetTime())),
                       1.0));
      }
    }
    auto pointLightsView = ecs.view<PointLight>();
    std::vector<PointLight> pointLights;
    pointLights.reserve(pointLightsView.size());
    pointLightsView.each([&pointLights](PointLight &pointLight) {
      pointLights.push_back(pointLight);
    });
    int num_max_point_lights = 128;
    struct {
      int num = 0;
      int garb[3] = {0, 0, 0};
    } initial_pointlight_data;
    initial_pointlight_data.num = pointLights.size();
    Buffer point_lights_buffer = Buffer(
        sizeof(int) * 4 + sizeof(glm::vec4) * 2 * num_max_point_lights, NULL);
    point_lights_buffer.modify(&initial_pointlight_data,
                               sizeof(initial_pointlight_data), 0);
    point_lights_buffer.modify(pointLights.data(),
                               sizeof(PointLight) * pointLights.size(),
                               sizeof(initial_pointlight_data));
    pointLights.clear();
    pointLights.shrink_to_fit();
    point_lights_buffer.bind_base(GL_SHADER_STORAGE_BUFFER, 6);

    std::array<uint64_t, 3> handles;
    int i = 0;
    for (auto tex : {gpos_tex, gnorm_tex, albedo_tex}) {
      ecs.get<Texture_2D>(tex).make_handle_resident();
      handles[i] = ecs.get<Texture_2D>(tex).get_handle();
      i++;
    }

    Buffer gbuffer_handles = Buffer(sizeof(uint64_t) * 3, handles.data());
    gbuffer_handles.bind_base(GL_UNIFORM_BUFFER, 5);
    gbuffer_handles_buf = ecs.create();
    ecs.emplace<Buffer>(gbuffer_handles_buf, gbuffer_handles);

    auto lighting_pass = Pipeline("src/Graphics/Shaders/LightingShader.vert",
                                  "src/Graphics/Shaders/LightingShader.frag");
    lighting_pass.change_depth_func(GL_ALWAYS);
    lighting_pass.add_pipeline_uniform_block("gbuffer");
    lighting_pass.bind_pipeline_uniform_block("gbuffer", 5);
    lighting_pass.add_pipeline_ssbo_block("point_lights_buffer");
    lighting_pass.bind_pipeline_ssbo_block("point_lights_buffer", 6);
    lighting_pass_pipe = ecs.create();
    ecs.emplace<Pipeline>(lighting_pass_pipe, lighting_pass);

    Texture_2D lighting_pass_out = Texture_2D(width, height, GL_RGBA);
    lighting_pass_out_tex = ecs.create();
    ecs.emplace<Texture_2D>(lighting_pass_out_tex, lighting_pass_out);
    lighting_pass_fbo = ecs.create();
    ecs.emplace<Framebuffer>(
        lighting_pass_fbo, width, height,
        std::vector<FBOattach>({FBOattach(lighting_pass_out, true)}),
        FBOattach(Renderbuffer(width, height, GL_DEPTH_COMPONENT16), false),
        DepthStencilAttachType::ONLY_DEPTH);
  }
  void execute(entt::registry &ecs) override {
    ecs.get<Pipeline>(lighting_pass_pipe).bind();
    ecs.get<Framebuffer>(lighting_pass_fbo).bind(GL_FRAMEBUFFER);
    glDrawArrays(GL_TRIANGLES, 0, 3);
  }
};