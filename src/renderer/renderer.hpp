#pragma once

#include "perdu/core/maths.hpp"
#include "perdu/renderer/gpu_context.hpp"
#include "perdu/renderer/mesh.hpp"

#include <cstdint>
#include <SDL3/SDL_gpu.h>
#include <vector>
namespace perdu {
	struct RenderState
	{
		uint32_t dim;
		uint32_t voff;
		uint32_t vcount;
	};

	struct TransformCache
	{
		Vectorf last_rot;
		Vectorf last_pos;

		std::vector<float> rotmat;
		bool			   _dirty = true;
	};

	struct EntityInfo
	{
		uint32_t voff;
		uint32_t vcount;
		uint32_t moff;
		uint32_t poff;
		float	 camera_dist = 2.0f;
		uint32_t dim;
		uint32_t _pad[2];
	};
}
