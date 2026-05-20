#pragma once

#include "perdu/assets/asset_cache.hpp"
#include "perdu/renderer/gpu_context.hpp"
#include "perdu/renderer/pipeline.hpp"
#include "perdu/renderer/shader.hpp"
#include "renderer/gpu_context.hpp"
#include "vulkan/vulkan.hpp"

#include <cstdint>
#include <SDL3/SDL_gpu.h>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace perdu {

	enum class PipelineType { Graphics, Compute };

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
		Pipeline(GPUContext*			   ctx,
				 PipelineType			   type,
				 std::vector<ShaderHandle> shaders,
				 Swapchain*				   swp);

		vk::raii::Pipeline& get() { return _pipeline; }

	  private:
		GPUContext*									   _ctx;
		std::vector<ShaderHandle>					   _shaders;
		std::vector<vk::PipelineShaderStageCreateInfo> _shaderstages;
		PipelineType								   _type;
		vk::raii::PipelineLayout					   _layout	 = nullptr;
		vk::raii::Pipeline							   _pipeline = nullptr;

		void create_graphics_pipeline(const vk::Format* swapchainformat);
		void create_compute_pipeline();
	};

	// class Pipeline {
	//   public:
	// 	Pipeline(GPUContext& ctx, const PipelineDesc& desc);
	// 	~Pipeline();
	//
	// 	Pipeline(Pipeline&&) noexcept;
	// 	Pipeline& operator=(Pipeline&&) noexcept;
	//
	// 	Pipeline(const Pipeline&)			 = delete;
	// 	Pipeline& operator=(const Pipeline&) = delete;
	//
	// 	void bind(SDL_GPURenderPass* pass) const;
	//
	// 	bool valid() const { return _pipeline != nullptr; }
	//
	//   private:
	// 	GPUContext&				 _ctx;
	// 	SDL_GPUGraphicsPipeline* _pipeline = nullptr;
	// };
}
