#include "perdu/core/assert.hpp"
#include "perdu/core/log.hpp"
#include "perdu/renderer/gpu_context.hpp"

#include <SDL3/SDL_gpu.h>

namespace perdu {
	void RenderTarget::create_depth_texture() {
		if (depth.valid()) destroy_depth_texture();

		SDL_GPUTextureCreateInfo info{
			.type				  = SDL_GPU_TEXTURETYPE_2D,
			.format				  = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
			.usage				  = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
			.width				  = width,
			.height				  = height,
			.layer_count_or_depth = 1,
			.num_levels			  = 1,
		};

		depth = { SDL_CreateGPUTexture(ctx.device, &info),
				  SDL_GPU_TEXTUREFORMAT_D32_FLOAT };
		PERDU_ASSERT(depth.valid(), "failed to create depth texture");
	}
	void RenderTarget::destroy_depth_texture() {
		if (depth.valid()) {
			SDL_ReleaseGPUTexture(ctx.device, depth.texture);
		} else {
			PERDU_LOG_WARN("tried to destroy non-existent depth texture");
		}
	}

	RenderTarget::~RenderTarget() {
		if (depth.valid()) destroy_depth_texture();
	}
}
