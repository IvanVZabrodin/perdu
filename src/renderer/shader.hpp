#pragma once

#include "perdu/assets/asset_cache.hpp"
#include "perdu/assets/assets.hpp"
#include "perdu/core/assert.hpp"
#include "perdu/renderer/gpu_context.hpp"
#include "perdu/renderer/shader.hpp"
#include "vulkan/vulkan.hpp"

#include <cstdint>
#include <map>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace perdu {
	std::vector<uint8_t> load_spirv(std::string path);

	struct GPUShader
	{
		GPUContext*				ctx;
		vk::ShaderStageFlagBits stage;
		vk::raii::ShaderModule	shader;

		vk::PipelineShaderStageCreateInfo to_pipelineinfo() const;
	};

	GPUShader load_shader(GPUContext* ctx,
						  std::string path,
						  ShaderStage stage,
						  uint32_t	  uniform_buffers,
						  uint32_t	  storage_buffers,
						  uint32_t	  samplers);

	GPUShader load_shader_from_code(GPUContext*			 ctx,
									std::vector<uint8_t> code,
									ShaderStage			 stage,
									uint32_t			 uniform_buffers,
									uint32_t			 storage_buffers,
									uint32_t			 samplers);

	GPUShader load_shader_from_cpushader(GPUContext* ctx, const CPUShader& cpu);


	struct CPUCompute
	{
		using RWPair = std::pair<uint32_t, uint32_t>;
		std::vector<uint32_t> spirv;
		RWPair				  buffcount;
		RWPair				  textcount;
		uint32_t			  uniformcount;
		uint32_t			  samplers;
		uint32_t			  threadx, thready, threadz;
	};

	class GPUCompute {
	  public:
		GPUCompute(GPUContext& ctx, const CPUCompute& cpu);
		~GPUCompute();

		SDL_GPUComputePipeline* get() const { return _pipeline; }

		void bind(SDL_GPUComputePass* pass);

		bool valid() const { return _pipeline; }

	  private:
		GPUContext&				_ctx;
		SDL_GPUComputePipeline* _pipeline;
	};

	using ComputeAsset = Asset<CPUCompute, GPUCompute>;

	ComputeAsset* generate_projection_shader(GPUContext& ctx, uint32_t dim);

	struct ComputeCache
	{
		std::map<uint32_t, ComputeAsset*> computes;
		GPUContext&						  ctx;

		ComputeAsset* get(uint32_t dim);

		ComputeCache(GPUContext& ctx);
		~ComputeCache();
	};

	void reflect(CPUShader& cpu);
}
