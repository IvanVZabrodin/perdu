#pragma once

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
	struct WinContext;

	struct GPUContext
	{
		SDL_GPUDevice* device = nullptr;

		GPUContext();
		~GPUContext();
	};

	struct RenderTarget
	{
		GPUContext&			 ctx;
		SDL_GPUTexture*		 colour = nullptr;
		SDL_GPUTextureFormat colour_format; // TODO: translate later
		SDL_GPUTexture*		 depth = nullptr;
		SDL_GPUTextureFormat depth_format;
		uint32_t			 width	 = 0;
		uint32_t			 height	 = 0;
		bool				 is_swap = false;

		virtual void load_texture(SDL_GPUCommandBuffer* cmd) {};

		void create_depth_texture();
		void destroy_depth_texture();

		RenderTarget(GPUContext& __ctx) : ctx(__ctx) {}
		~RenderTarget();
	};


	struct RenderView
	{
		Camera*		  camera;
		RenderTarget* target;
		// TODO: viewport
	};

	struct WinTarget : public RenderTarget
	{
		SDL_Window* window = nullptr;

		WinTarget(GPUContext& ctx) : RenderTarget(ctx) {}

		void load_texture(SDL_GPUCommandBuffer* cmd) {
			if (window == nullptr) return;
			SDL_AcquireGPUSwapchainTexture(
			  cmd, window, &colour, nullptr, nullptr);
			colour_format
			  = SDL_GetGPUSwapchainTextureFormat(ctx.device, window);
		}
	};


}
