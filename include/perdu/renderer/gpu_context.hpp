#pragma once

#include "entt/entity/fwd.hpp"
#include "perdu/components/camera.hpp"
#include "perdu/core/log.hpp"

#include <cstdint>
#include <SDL3/SDL_gpu.h>
#include <string>
#include <vector>
struct SDL_GPUDevice;
struct SDL_Window;
struct SDL_GPUTexture;
struct SDL_GPUCommandBuffer;

namespace perdu {
	struct GPUContext
	{
		SDL_GPUDevice* device = nullptr;

		GPUContext();
		~GPUContext();
	};

	struct GPUTexture
	{
		SDL_GPUTexture*		 texture = nullptr;
		SDL_GPUTextureFormat format;

		bool valid() const { return texture != nullptr; }
	};

	struct RenderTarget
	{
		GPUContext& ctx;
		GPUTexture	colour;
		GPUTexture	depth;
		uint32_t	width	= 0;
		uint32_t	height	= 0;
		bool		is_swap = false;

		virtual GPUTexture load_texture(SDL_GPUCommandBuffer* cmd) {
			return colour;
		}

		void create_depth_texture();
		void destroy_depth_texture();

		RenderTarget(GPUContext& __ctx) : ctx(__ctx) {}
		~RenderTarget();
	};

	struct WinTarget : public RenderTarget
	{
		SDL_Window* window = nullptr;

		WinTarget(GPUContext& ctx) : RenderTarget(ctx) {}

		GPUTexture load_texture(SDL_GPUCommandBuffer* cmd) {
			if (window == nullptr) return {};
			SDL_AcquireGPUSwapchainTexture(
			  cmd, window, &colour.texture, nullptr, nullptr);
			colour.format
			  = SDL_GetGPUSwapchainTextureFormat(ctx.device, window);
			return colour;
		}
	};

	struct RenderView
	{
		entt::entity  camera;
		RenderTarget* target;
		// TODO: viewport
	};
}
