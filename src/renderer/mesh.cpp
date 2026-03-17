#include "perdu/renderer/mesh.hpp"

#include "perdu/core/assert.hpp"
#include "perdu/renderer/gpu_context.hpp"

#include <cstdint>
#include <SDL3/SDL_gpu.h>

namespace perdu {
	void GPUMesh::ensure_indbuf(uint32_t size) {
		if (isize <= size) create_indbuf(size);
	}

	void GPUMesh::create_indbuf(uint32_t size) {
		if (inds) SDL_ReleaseGPUBuffer(ctx.device, inds);
		SDL_GPUBufferCreateInfo info{ .usage = SDL_GPU_BUFFERUSAGE_INDEX,
									  .size	 = size };
		isize = size;
		inds  = SDL_CreateGPUBuffer(ctx.device, &info);
	}

	GPUMesh::~GPUMesh() {
		if (inds) SDL_ReleaseGPUBuffer(ctx.device, inds);
	}

}
