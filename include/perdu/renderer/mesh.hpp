#pragma once

#include "perdu/renderer/gpu_context.hpp"

#include <cstdint>
#include <vector>
struct SDL_GPUBuffer;
struct SDL_GPUTransferBuffer;

namespace perdu {
	enum class PrimitiveType { Triangles, Lines, Points };

	struct CPUMesh
	{
		uint32_t			  dim;
		PrimitiveType		  primitive_type;
		std::vector<float>	  vertices{};
		std::vector<uint32_t> indices{};
		bool				  dirty = false;
	};

	struct GPUMesh
	{
		GPUContext&	   ctx;
		SDL_GPUBuffer* inds	 = nullptr;
		uint32_t	   isize = 0;

		void ensure_indbuf(uint32_t size);

		void create_indbuf(uint32_t size);

		~GPUMesh();
	};
}
