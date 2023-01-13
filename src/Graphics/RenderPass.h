#pragma once
#include "Entity/Entity.h"
#include "GLFW/glfw3.h"
#include "HigherGraphics_2.h"
#include "Lights.h"
#include "Mesh/Mesh.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/fwd.hpp"
#include <algorithm>
#include <cstdint>
#include <vector>

class GeometryPass : public RenderPass {
  typedef FramebufferAttachment FBOattach;

private:
  Entity proj_view_buf;
  Entity models_buf;
  Entity material_ids_buf;
  Entity indirect_draw_buf;

  int width;
  int height;

public:
  Entity g_pos_tex;
  Entity g_norm_tex;
  Entity g_buffer_fbo;
  Entity material_depth_tex;
  Entity geometry_pipe;

  GeometryPass(int width, int height, Entity indirect_draw_buf,
               Entity proj_view_buf, Entity models_buf, Entity material_ids_buf)
      : width(width), height(height), indirect_draw_buf(indirect_draw_buf),
        proj_view_buf(proj_view_buf), models_buf(models_buf),
        material_ids_buf(material_ids_buf) {}

  void init(ECSmanager &ecs) override {
    Texture_2D g_pos = Texture_2D(width, height, GL_RGB32F);
    g_pos.make_handle_resident();
    g_pos_tex = ecs.new_entity(g_pos);
    Texture_2D g_norm = Texture_2D(width, height, GL_RGB32F);
    g_norm.make_handle_resident();
    g_norm_tex = ecs.new_entity(g_norm);
    Texture_2D material_depth = Texture_2D(width, height, GL_RGB32F);
    material_depth.make_handle_resident();
    material_depth_tex = ecs.new_entity(material_depth);
    Renderbuffer geometry_pass_depth =
        Renderbuffer(width, height, GL_DEPTH24_STENCIL8);

    Framebuffer g_buffer(width, height,
                         {FBOattach(g_pos, true), FBOattach(g_norm, true),
                          FBOattach(material_depth, true)},
                         FBOattach(geometry_pass_depth, false),
                         DepthStencilAttachType::BOTH_DEPTH_STENCIL);
    g_buffer_fbo = ecs.new_entity(g_buffer);

    // indirect_draw_buf = renderer_entities["indirect_draw_buf"];
    // proj_view_buf = renderer_entities["proj_view_buf"];
    // models_buf = renderer_entities["models_buf"];
    // material_ids_buf = renderer_entities["material_ids_buf"];

    Pipeline geometryBase = Pipeline(R"(src/Graphics/Shaders/newDraw.vert)",
                                     R"(src/Graphics/Shaders/newDraw.frag)");
    geometry_pipe = ecs.new_entity(geometryBase);
    auto geometry = ecs.get_component<Pipeline>(geometry_pipe);
    geometryBase = *geometry;
    geometry->add_pipeline_uniform_block("Matrices");
    geometry->bind_pipeline_uniform_block("Matrices", 1);
    geometry->add_pipeline_ssbo_block("Transformations");
    geometry->bind_pipeline_ssbo_block("Transformations", 2);
    geometry->add_pipeline_ssbo_block("MaterialIds");
    geometry->bind_pipeline_ssbo_block("MaterialIds", 3);

    // renderer_entities["g_pos_tex"] = g_pos_tex;
    // renderer_entities["g_norm_tex"] = g_norm_tex;
    // renderer_entities["material_depth_tex"] = material_depth_tex;
    // renderer_entities["geometry_pipe"] = geometry_pipe;
  }
  void execute(ECSmanager &ecs) override {
    auto g_buffer = *ecs.get_component<Framebuffer>(g_buffer_fbo);
    auto geometry = *ecs.get_component<Pipeline>(geometry_pipe);
    auto indirect_draw_buffer = *ecs.get_component<Buffer>(indirect_draw_buf);
    g_buffer.bind(GL_FRAMEBUFFER);
    g_buffer.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    geometry.bind();

    indirect_draw_buffer.bind(GL_DRAW_INDIRECT_BUFFER);
    MeshStatic::get_static_meshes_holder().multi_draw();
  }
};

class TextureToDepthPass : public RenderPass {
private:
  Entity material_depth_tex;
  Entity material_pass_fbo;

public:
  Entity tex_to_depth_pipe;

  TextureToDepthPass(Entity material_depth_tex, Entity material_pass_fbo)
      : material_depth_tex(material_depth_tex),
        material_pass_fbo(material_pass_fbo) {}
  void init(ECSmanager &ecs) override {
    Pipeline tex_to_depth =
        Pipeline(R"(src/Graphics/Shaders/FullScreen.vert)",
                 R"(src/Graphics/Shaders/TextureToDepth.frag)");
    tex_to_depth.change_depth_func(GL_ALWAYS);
    tex_to_depth.set_uniform<uint64_t>(
        tex_to_depth.get_uniform_loc("depth_texture"),
        ecs.get_component<Texture_2D>(material_depth_tex)->get_handle());
    tex_to_depth_pipe = ecs.new_entity(tex_to_depth);
  }
  void execute(ECSmanager &ecs) override {
    auto tex_to_depth = ecs.get_component<Pipeline>(tex_to_depth_pipe);
    auto material_pass_framebuff =
        ecs.get_component<Framebuffer>(material_pass_fbo);
    material_pass_framebuff->bind(GL_FRAMEBUFFER);
    material_pass_framebuff->clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    tex_to_depth->bind();
    glDrawArrays(GL_TRIANGLES, 0, 3);
  }
};

class MaterialPass : public RenderPass {
  typedef FramebufferAttachment FBOattach;

private:
  Entity materials_buf;
  Entity active_material_id_buf;
  Entity materials_vec;
  int width;
  int height;

public:
  Entity material_pipe;
  Entity material_pass_fbo;
  Entity material_pass_out_tex;
  Entity current_material_info_buffer;

  MaterialPass(int width, int height, Entity materials_buf,
               Entity active_material_id_buf, Entity materials_vec)
      : width(width), height(height), materials_buf(materials_buf),
        active_material_id_buf(active_material_id_buf),
        materials_vec(materials_vec) {}

  void init(ECSmanager &ecs) override {
    Pipeline material = Pipeline(R"(src/Graphics/Shaders/MaterialShader.vert)",
                                 R"(src/Graphics/Shaders/MaterialShader.frag)");
    material.change_depth_func(GL_EQUAL);

    material.add_pipeline_uniform_block("MaterialID");
    material.bind_pipeline_uniform_block("MaterialID", 2);
    material.add_pipeline_uniform_block("Materials");
    material.bind_pipeline_uniform_block("Materials", 4);

    // ecs.get_component<Buffer>(materials_buf)
    //     ->bind_base(GL_SHADER_STORAGE_BUFFER,
    //                 material.get_pipeline_ssbo_block_binding("Materials"));
    ecs.get_component<Buffer>(active_material_id_buf)
        ->bind_base(GL_UNIFORM_BUFFER,
                    material.get_pipeline_uniform_block_binding("MaterialID"));

    material_pipe = ecs.new_entity(material);

    Texture_2D material_pass_out = Texture_2D(width, height, GL_RGBA);
    material_pass_out_tex = ecs.new_entity(material_pass_out);
    Renderbuffer material_pass_depth =
        Renderbuffer(width, height, GL_DEPTH_COMPONENT32F);
    material_pass_fbo = ecs.new_entity(
        Framebuffer(width, height, {FBOattach(material_pass_out, true)},
                    FBOattach(material_pass_depth, false),
                    DepthStencilAttachType::ONLY_DEPTH));

    Buffer current_material_info(sizeof(LoadedMaterialPBR::BufferDataPBR),
                                 NULL);
    current_material_info.bind_base(
        GL_UNIFORM_BUFFER,
        material.get_pipeline_uniform_block_binding("Materials"));
    current_material_info_buffer = ecs.new_entity(current_material_info);
  }

  void execute(ECSmanager &ecs) override {
    auto current_material_info = ecs.get_component<Buffer>(current_material_info_buffer);
    auto materials_vector = ecs.get_component<std::vector<LoadedMaterialPBR>>(materials_vec); 
    auto material = ecs.get_component<Pipeline>(material_pipe);
    material->bind();
    ecs.get_component<Framebuffer>(material_pass_fbo)->bind(GL_FRAMEBUFFER);
    for (uint32_t i = 0; i < materials_vector->size(); ++i) {
      // update active mat id
      uint32_t material_id = i + 1;
      ecs.get_component<Buffer>(active_material_id_buf)
          ->modify(&material_id, sizeof(uint32_t), 0);
      auto data = materials_vector->at(i).getBufferData();
      current_material_info->modify(&data, sizeof(data), 0);
      glDrawArrays(GL_TRIANGLES, 0, 3);
    }
  }
};

class LightingPass : public RenderPass {
  typedef FramebufferAttachment FBOattach;

private:
  Entity gpos_tex;
  Entity gnorm_tex;
  Entity albedo_tex;
  int width;
  int height;

public:
  Entity gbuffer_handles_buf;
  Entity nonshadow_point_lights_buf;
  Entity lighting_pass_pipe;
  Entity lighting_pass_fbo;
  Entity lighting_pass_out_tex;

  LightingPass(Entity gpos_tex, Entity gnorm_tex, Entity albedo_tex, int width,
               int height)
      : gpos_tex(gpos_tex), gnorm_tex(gnorm_tex), albedo_tex(albedo_tex),
        width(width), height(height) {}

  void init(ECSmanager &ecs) override {
    ecs.register_component<PointLight>();
    for (int x = -3; x < 3; x++) {
      for (int y = -2; y < 2; y++) {
        ecs.new_entity(
            PointLight(glm::vec3(3.0 * x, 4.0, 2.0 * y), 5.0,
                       glm::vec3(sin(glfwGetTime() * x), cos(glfwGetTime() * y),
                                 sin(glfwGetTime()) > cos(glfwGetTime())
                                     ? sin(glfwGetTime())
                                     : cos(glfwGetTime())),
                       1.0));
      }
    }
    // ecs.new_entity(
    // PointLight(glm::vec3(0.5, 4.0, 0.4), 5.0, glm::vec3(1.0), 1.0));
    auto pointLights = ecs.get_components_of_type<PointLight>();

    int num_max_point_lights = 128;
    struct {
      int num = 0;
      int garb[3] = {0, 0, 0};
      // glm::vec4 position_radius = glm::vec4(0.5,4.0,0.4,5.0);
      // glm::vec4 color_int = glm::vec4(1.0,1.0,1.0,1.0);
    } initial_point_light;
    initial_point_light.num = pointLights.size();
    Buffer point_lights_buffer = Buffer(
        sizeof(int) * 4 + sizeof(glm::vec4) * 2 * num_max_point_lights, NULL);
    point_lights_buffer.modify(&initial_point_light,
                               sizeof(initial_point_light), 0);
    point_lights_buffer.modify(pointLights.get_array_ptr(),
                               sizeof(PointLight) * pointLights.size(),
                               sizeof(initial_point_light));
    point_lights_buffer.bind_base(GL_SHADER_STORAGE_BUFFER, 6);

    std::array<uint64_t, 3> handles;
    int i = 0;
    for (auto tex : {gpos_tex, gnorm_tex, albedo_tex}) {
      ecs.get_component<Texture_2D>(tex)->make_handle_resident();
      handles[i] = ecs.get_component<Texture_2D>(tex)->get_handle();
      i++;
    }

    Buffer gbuffer_handles = Buffer(sizeof(uint64_t) * 3, handles.data());
    gbuffer_handles.bind_base(GL_UNIFORM_BUFFER, 5);
    gbuffer_handles_buf = ecs.new_entity(gbuffer_handles);

    auto lighting_pass = Pipeline("src/Graphics/Shaders/LightingShader.vert",
                                  "src/Graphics/Shaders/LightingShader.frag");
    lighting_pass.change_depth_func(GL_ALWAYS);
    lighting_pass.add_pipeline_uniform_block("gbuffer");
    lighting_pass.bind_pipeline_uniform_block("gbuffer", 5);
    lighting_pass.add_pipeline_ssbo_block("point_lights_buffer");
    lighting_pass.bind_pipeline_ssbo_block("point_lights_buffer", 6);
    lighting_pass_pipe = ecs.new_entity(lighting_pass);

    Texture_2D lighting_pass_out = Texture_2D(width, height, GL_RGBA);
    lighting_pass_out_tex = ecs.new_entity(lighting_pass_out);
    lighting_pass_fbo = ecs.new_entity(Framebuffer(
        width, height, {FBOattach(lighting_pass_out, true)},
        FBOattach(Renderbuffer(width, height, GL_DEPTH_COMPONENT16), false),
        DepthStencilAttachType::ONLY_DEPTH));
  }
  void execute(ECSmanager &ecs) override {
    ecs.get_component<Pipeline>(lighting_pass_pipe)->bind();
    ecs.get_component<Framebuffer>(lighting_pass_fbo)->bind(GL_FRAMEBUFFER);
    glDrawArrays(GL_TRIANGLES, 0, 3);
  }
};