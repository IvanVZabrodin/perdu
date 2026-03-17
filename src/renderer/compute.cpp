#include "perdu/core/assert.hpp"
#include "perdu/core/log.hpp"
#include "perdu/renderer/gpu_context.hpp"
#include "renderer/shader.hpp"

#include <cstdint>
#include <SDL3/SDL_gpu.h>
#include <vector>

namespace perdu {
	GPUCompute::GPUCompute(GPUContext& ctx, const CPUCompute& cpu) : _ctx(ctx) {
		SDL_GPUComputePipelineCreateInfo info{
			.code_size	  = cpu.spirv.size() * sizeof(uint32_t),
			.code		  = reinterpret_cast<const uint8_t*>(cpu.spirv.data()),
			.entrypoint	  = "main",
			.format		  = SDL_GPU_SHADERFORMAT_SPIRV,
			.num_samplers = cpu.samplers,
			.num_readonly_storage_textures	= cpu.textcount.first,
			.num_readonly_storage_buffers	= cpu.buffcount.first,
			.num_readwrite_storage_textures = cpu.textcount.second,
			.num_readwrite_storage_buffers	= cpu.buffcount.second,
			.num_uniform_buffers			= cpu.uniformcount,
			.threadcount_x					= cpu.threadx,
			.threadcount_y					= cpu.thready,
			.threadcount_z					= cpu.threadz,
		};


		_pipeline = SDL_CreateGPUComputePipeline(_ctx.device, &info);
		PERDU_ASSERT(_pipeline, "failed to create compute pipeline");
	}

	GPUCompute::~GPUCompute() {
		if (_pipeline) SDL_ReleaseGPUComputePipeline(_ctx.device, _pipeline);
	}

	void GPUCompute::bind(SDL_GPUComputePass* pass) {
		SDL_BindGPUComputePipeline(pass, _pipeline);
	}
}
