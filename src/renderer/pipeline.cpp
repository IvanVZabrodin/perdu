#include "renderer/pipeline.hpp"

#include "perdu/core/log.hpp"
#include "perdu/renderer/gpu_context.hpp"
#include "perdu/renderer/pipeline.hpp"
#include "perdu/renderer/shader.hpp"

#include <cstdint>
#include <SDL3/SDL_gpu.h>
#include <vector>

static SDL_GPUVertexElementFormat
  to_sdlformat(perdu::VertexAttribute::Format format) {
	switch (format) {
		case (perdu::VertexAttribute::Format::Float2):
			return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
		case (perdu::VertexAttribute::Format::Float3):
			return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
		case (perdu::VertexAttribute::Format::Float4):
			return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
		case (perdu::VertexAttribute::Format::Float):
			return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
	}
}

static SDL_GPUPrimitiveType to_sdlprimitive(perdu::PrimitiveType type) {
	switch (type) {
		case (perdu::PrimitiveType::Triangles):
			return SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
		case (perdu::PrimitiveType::Lines):
			return SDL_GPU_PRIMITIVETYPE_LINELIST;
		case (perdu::PrimitiveType::Points):
			return SDL_GPU_PRIMITIVETYPE_POINTLIST;
	}
}

namespace perdu {
	Pipeline::Pipeline(GPUContext& ctx, const PipelineDesc& desc) : _ctx(ctx) {
		std::vector<SDL_GPUVertexAttribute> attrs;
		for (auto& a : desc.attributes) {
			attrs.push_back({ .location = a.location,
							  .format	= to_sdlformat(a.format),
							  .offset	= a.offset });
		}

		SDL_GPUVertexBufferDescription vbd{ .slot  = 0,
											.pitch = desc.vertex_stride,
											.input_rate
											= SDL_GPU_VERTEXINPUTRATE_VERTEX };

		SDL_GPUVertexInputState vertex_input{ .vertex_buffer_descriptions
											  = &vbd,
											  .num_vertex_buffers = 1,
											  .vertex_attributes = attrs.data(),
											  .num_vertex_attributes
											  = (uint32_t) attrs.size() };

		SDL_GPUColorTargetDescription color_target{ .format
													= desc.colour_format };

		SDL_GPUGraphicsPipelineCreateInfo info {
			.vertex_shader = desc.vert->gpu.handle(),
			.fragment_shader = desc.frag->gpu.handle(),
			.vertex_input_state = vertex_input,
			.primitive_type = to_sdlprimitive(desc.primitive_type),
			.rasterizer_state = {
				.cull_mode = SDL_GPU_CULLMODE_NONE
			},
			.depth_stencil_state = {
				.compare_op = SDL_GPU_COMPAREOP_GREATER,
				.enable_depth_test = desc.depth_test,
				.enable_depth_write = desc.depth_write,
			},
			.target_info = {
				.color_target_descriptions = &color_target,
				.num_color_targets = 1,
				.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
				.has_depth_stencil_target = true
			},
		};

		_pipeline = SDL_CreateGPUGraphicsPipeline(ctx.device, &info);
		PERDU_ASSERT(_pipeline, "failed to create pipeline");

		PERDU_LOG_INFO("pipeline created");
	}

	Pipeline::~Pipeline() {
		if (_pipeline) SDL_ReleaseGPUGraphicsPipeline(_ctx.device, _pipeline);
	}

	void Pipeline::bind(SDL_GPURenderPass* pass) const {
		PERDU_ASSERT(_pipeline, "trying to bind to invalid pipeline");
		SDL_BindGPUGraphicsPipeline(pass, _pipeline);
	}

	Pipeline::Pipeline(Pipeline&& other) noexcept :
		_ctx(other._ctx), _pipeline(other._pipeline) {
		other._pipeline = nullptr;
	}

	Pipeline& Pipeline::operator=(Pipeline&& other) noexcept {
		if (this == &other) return *this;
		if (_pipeline) SDL_ReleaseGPUGraphicsPipeline(_ctx.device, _pipeline);
		_pipeline		= other._pipeline;
		other._pipeline = nullptr;
		return *this;
	}

}
