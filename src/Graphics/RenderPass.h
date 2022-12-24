#pragma once
#include "Entity/Entity.h"
#include "Graphics/Graphics_2.h"
#include "HigherGraphics_2.h"


class GeometryPass : public RenderPass {
typedef FramebufferAttachment FBOattach;
private:    

    Entity g_pos_tex;
    Entity g_norm_tex;
    Entity g_buffer_fbo;
    Entity material_depth_tex;
    Entity geometry_pipe;

    Entity proj_view_buf;
    Entity models_buf;
    Entity material_ids_buf;
    Entity indirect_draw_buf;

    int width;
    int height;

public:

    GeometryPass(int width, int height) : width(width), height(height) {}

    void init(ECSmanager &ecs, std::unordered_map<std::string, Entity> &renderer_entities) override {
		Texture_2D g_pos = Texture_2D(width, height, GL_RGB32F);
        g_pos.make_handle_resident();
        g_pos_tex = ecs.new_entity(g_pos);
		Texture_2D g_norm = Texture_2D(width, height, GL_RGB32F);
        g_norm.make_handle_resident();
		g_norm_tex = ecs.new_entity(g_norm);
        Texture_2D material_depth = Texture_2D(width, height, GL_RGB32F);
        material_depth.make_handle_resident();
        material_depth_tex = ecs.new_entity(material_depth);
		Renderbuffer geometry_pass_depth = Renderbuffer(width, height, GL_DEPTH24_STENCIL8);

	Framebuffer g_buffer(width, height,
		{ FBOattach(g_pos, true),
			FBOattach(g_norm, true),
			FBOattach(material_depth, true) },
		FBOattach(geometry_pass_depth, false), DepthStencilAttachType::BOTH_DEPTH_STENCIL
	);
    g_buffer_fbo = ecs.new_entity(g_buffer);

    indirect_draw_buf = renderer_entities["indirect_draw_buf"];
    proj_view_buf = renderer_entities["proj_view_buf"];
    models_buf = renderer_entities["models_buf"];
    material_ids_buf = renderer_entities["material_ids_buf"];

    Pipeline geometryBase = Pipeline(R"(src/Graphics/Shaders/newDrawVert.glsl)", R"(src/Graphics/Shaders/newDrawFrag.glsl)");
    geometry_pipe = ecs.new_entity(geometryBase);
    auto geometry = ecs.get_component<Pipeline>(geometry_pipe);
    geometryBase = *geometry;
	geometry->add_pipeline_uniform_block("Matrices");
	geometry->bind_pipeline_uniform_block("Matrices", 1);
	geometry->add_pipeline_ssbo_block("Transformations");
	geometry->bind_pipeline_ssbo_block("Transformations", 2);
	geometry->add_pipeline_ssbo_block("MaterialIds");
	geometry->bind_pipeline_ssbo_block("MaterialIds", 3);

    renderer_entities["g_pos_tex"] = g_pos_tex;
    renderer_entities["g_norm_tex"] = g_norm_tex;
    renderer_entities["material_depth_tex"] = material_depth_tex;
    renderer_entities["geometry_pipe"] = geometry_pipe;
}
    void execute(ECSmanager &ecs, std::unordered_map<std::string, Entity> &renderer_entities) override {
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
