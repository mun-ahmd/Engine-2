#pragma once
#include "Entity/Entity.h"
#include "Graphics/Graphics_2.h"
#include "HigherGraphics_2.h"
#include "glm/ext/matrix_float4x4.hpp"

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

    Pipeline geometryBase =
        Pipeline(R"(src/Graphics/Shaders/newDrawVert.glsl)",
                 R"(src/Graphics/Shaders/newDrawFrag.glsl)");
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
        Pipeline(R"(src/Graphics/Shaders/FullScreenVert.glsl)",
                 R"(src/Graphics/Shaders/TextureToDepthFrag.glsl)");
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
  Entity shadow_map_tex;
  Entity active_material_id_buf;
  Entity g_buffer_handles_buf;
  Entity lightspacetrans_entity;
  Entity materials_vec;
  int width;
  int height;

public:
  Entity material_pipe;
  Entity material_pass_fbo;
  Entity material_pass_out_tex;

  MaterialPass(int width, int height, Entity materials_buf,
               Entity shadow_map_tex, Entity active_material_id_buf,
               Entity g_buffer_handles_buf, Entity lightspacetrans,
               Entity materials_vec)
      : width(width), height(height), materials_buf(materials_buf),
        shadow_map_tex(shadow_map_tex),
        active_material_id_buf(active_material_id_buf),
        g_buffer_handles_buf(g_buffer_handles_buf),
        lightspacetrans_entity(lightspacetrans), materials_vec(materials_vec) {}

  void init(ECSmanager &ecs) override {
    Pipeline material =
        Pipeline(R"(src/Graphics/Shaders/MaterialShaderVert.glsl)",
                 R"(src/Graphics/Shaders/MaterialShader.glsl)");
    material.change_depth_func(GL_EQUAL);

    material.add_pipeline_uniform_block("MaterialID");
    material.bind_pipeline_uniform_block("MaterialID", 2);
    material.add_pipeline_uniform_block("gbuffer");
    material.bind_pipeline_uniform_block("gbuffer", 3);
    material.add_pipeline_ssbo_block("Materials");
    material.bind_pipeline_ssbo_block("Materials", 4);
    material.set_uniform<uint64_t>(
        material.get_uniform_loc("shadow_map"),
        ecs.get_component<Texture_2D>(shadow_map_tex)->get_handle());

    ecs.get_component<Buffer>(materials_buf)
        ->bind_base(GL_SHADER_STORAGE_BUFFER,
                    material.get_pipeline_ssbo_block_binding("Materials"));
    ecs.get_component<Buffer>(active_material_id_buf)
        ->bind_base(GL_UNIFORM_BUFFER,
                    material.get_pipeline_uniform_block_binding("MaterialID"));
    ecs.get_component<Buffer>(g_buffer_handles_buf)
        ->bind_base(GL_UNIFORM_BUFFER,
                    material.get_pipeline_uniform_block_binding("gbuffer"));

    glm::vec3 LIGHTPOS = glm::vec3(0.5, 1, 1) * 10.f;
    material.set_uniform_vec<3, float>(material.get_uniform_loc("lightPos"),
                                       (float *)&LIGHTPOS);

    material_pipe = ecs.new_entity(material);

    Texture_2D material_pass_out = Texture_2D(width, height, GL_RGBA);
    material_pass_out_tex = ecs.new_entity(material_pass_out);
    Renderbuffer material_pass_depth =
        Renderbuffer(width, height, GL_DEPTH_COMPONENT32F);
    material_pass_fbo = ecs.new_entity(
        Framebuffer(width, height, {FBOattach(material_pass_out, true)},
                    FBOattach(material_pass_depth, false),
                    DepthStencilAttachType::ONLY_DEPTH));
  }

  void execute(ECSmanager &ecs) override {
    auto material = ecs.get_component<Pipeline>(material_pipe);
    material->bind();
    ecs.get_component<Framebuffer>(material_pass_fbo)->bind(GL_FRAMEBUFFER);
    glm::mat4 lightspacetrans =
        *ecs.get_component<glm::mat4>(lightspacetrans_entity);
    std::vector<glm::vec4> materials =
        *ecs.get_component<std::vector<glm::vec4>>(materials_vec);
    material->set_uniform_mat<4, 4>(material->get_uniform_loc("lightspace"),
                                    glm::value_ptr(lightspacetrans));
    for (uint32_t i = 0; i < materials.size(); ++i) {
      // update active mat id
      uint32_t material_id = i + 1;
      ecs.get_component<Buffer>(active_material_id_buf)
          ->modify(&material_id, sizeof(uint32_t), 0);
      glDrawArrays(GL_TRIANGLES, 0, 3);
    }
  }
};