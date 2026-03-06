#include "perdu/renderer/renderer.hpp"

#include "perdu/core/assert.hpp"
#include "perdu/core/log.hpp"
#include "perdu/renderer/gpu_context.hpp"

#include <cstdint>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

namespace perdu {
	Renderer::Renderer(GPUContext& ctx) : _ctx(ctx) {
		create_depth_texture();

		PERDU_LOG_INFO("renderer initialised");
	}

	Renderer::~Renderer() {
		destroy_depth_texture();

		PERDU_LOG_INFO("renderer destroyed");
	}

	void Renderer::on_resize(uint32_t width, uint32_t height) {
		PERDU_LOG_DEBUG("reacting to resize");

		create_depth_texture();
	}

	void Renderer::begin_frame() {}
	void Renderer::end_frame() {}

	void Renderer::create_depth_texture() {
		if (_ctx.depth_texture) destroy_depth_texture();

		SDL_GPUTextureCreateInfo info{
			.type				  = SDL_GPU_TEXTURETYPE_2D,
			.format				  = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
			.usage				  = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
			.width				  = _ctx.width,
			.height				  = _ctx.height,
			.layer_count_or_depth = 1,
			.num_levels			  = 1,
		};

		_ctx.depth_texture = SDL_CreateGPUTexture(_ctx.device, &info);
		PERDU_ASSERT(_ctx.depth_texture, "failed to create depth texture");
	}

	void Renderer::destroy_depth_texture() {
		if (_ctx.depth_texture) {
			SDL_ReleaseGPUTexture(_ctx.device, _ctx.depth_texture);
		} else {
			PERDU_LOG_WARN("tried to destroy non-existent depth texture");
		}
	}
}
