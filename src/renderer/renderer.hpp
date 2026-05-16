#pragma once

#include "perdu/assets/asset_cache.hpp"
#include "perdu/components/material.hpp"
#include "perdu/components/transform.hpp"
#include "perdu/core/maths.hpp"
#include "perdu/renderer/gpu_context.hpp"
#include "perdu/renderer/mesh.hpp"
#include "perdu/renderer/renderer.hpp"

#include <cstdint>
#include <SDL3/SDL_gpu.h>
#include <vector>
namespace perdu {
	struct RenderState
	{
		uint32_t	  dim;
		bool		  allocated = false;
		RenderOffsets offsets;

		uint32_t vcount;
	};

	struct TransformCache
	{
		Vectorf last_rot;
		Vectorf last_pos;

		std::vector<float> rotmat;
		bool			   _dirty = true;

		bool compare(const Transform& t);
	};

	struct MaterialCache
	{
		ShaderHandle last_vert;
		ShaderHandle last_frag;

		bool _dirty = true;

		bool compare(const Material& m) const;
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

	struct CameraData
	{
		std::vector<float> mat;
		std::vector<float> tran;
	};

	struct CameraInfo
	{
		float* mat;
		float* tran;
	};
}
