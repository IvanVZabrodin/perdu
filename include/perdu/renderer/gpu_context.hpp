#pragma once

#include <cstdint>
struct SDL_GPUDevice;
struct SDL_Window;
struct SDL_GPUTexture;

namespace perdu {
	struct GPUContext
	{
		SDL_GPUDevice*	device		  = nullptr;
		SDL_Window*		window		  = nullptr;
		uint32_t		height		  = 800;
		uint32_t		width		  = 600;
		SDL_GPUTexture* depth_texture = nullptr;
	};
}
