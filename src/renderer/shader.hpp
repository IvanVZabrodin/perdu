#pragma once

#include "perdu/assets/asset_cache.hpp"
#include "perdu/assets/assets.hpp"
#include "perdu/core/assert.hpp"
#include "perdu/renderer/gpu_context.hpp"
#include "perdu/renderer/shader.hpp"

#include <cstdint>
#include <map>
#include <SDL3/SDL_gpu.h>
#include <vector>

namespace perdu {
	std::vector<uint8_t> load_spirv(std::string path);

	SDL_GPUShader* load_sdlshader(SDL_GPUDevice*	 device,
								  std::string		 path,
								  SDL_GPUShaderStage stage,
								  uint32_t			 uniform_buffers,
								  uint32_t			 storage_buffers,
								  uint32_t			 samplers);

	SDL_GPUShader* load_sdlshader_from_code(SDL_GPUDevice*		 device,
											std::vector<uint8_t> code,
											SDL_GPUShaderStage	 stage,
											uint32_t uniform_buffers,
											uint32_t storage_buffers,
											uint32_t samplers);


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
