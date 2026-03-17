#include "perdu/renderer/gpu_context.hpp"

#include "perdu/core/assert.hpp"
#include "perdu/core/log.hpp"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_init.h>

namespace perdu {
	GPUContext::GPUContext() {
		static bool sdl_init = false;
		if (!sdl_init) {
			SDL_Init(SDL_INIT_VIDEO);
			sdl_init = true;
		}

#ifdef NDEBUG
		device
		  = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, nullptr);
#else
		device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, nullptr);
#endif

		PERDU_ASSERT(device, "gpu device failed to initialise");
		PERDU_LOG_DEBUG("using device driver "
						+ std::string(SDL_GetGPUDeviceDriver(device)));
	}

	GPUContext::~GPUContext() {
		if (device) SDL_DestroyGPUDevice(device);
	}
}
