#pragma once

#include "perdu/assets/asset_cache.hpp"
#include "perdu/renderer/gpu_context.hpp"
#include "perdu/renderer/pipeline.hpp"
#include "perdu/renderer/shader.hpp"

#include <cstdint>
#include <SDL3/SDL_gpu.h>
#include <vector>

namespace perdu {

	struct PipelineDesc
	{
		ShaderHandle				 vert;
		ShaderHandle				 frag;
		std::vector<VertexAttribute> attributes;
		uint32_t					 vertex_stride;
		SDL_GPUTextureFormat		 colour_format;
		PrimitiveType				 primitive_type = PrimitiveType::Triangles;
		bool						 depth_test		= true;
		bool						 depth_write	= true;
	};

	class Pipeline {
	  public:
		Pipeline(GPUContext& ctx, const PipelineDesc& desc);
		~Pipeline();

		Pipeline(Pipeline&&) noexcept;
		Pipeline& operator=(Pipeline&&) noexcept;

		Pipeline(const Pipeline&)			 = delete;
		Pipeline& operator=(const Pipeline&) = delete;

		void bind(SDL_GPURenderPass* pass) const;

		bool valid() const { return _pipeline != nullptr; }

	  private:
		GPUContext&				 _ctx;
		SDL_GPUGraphicsPipeline* _pipeline = nullptr;
	};
}
