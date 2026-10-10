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
		uint32_t mesh_offset;
		uint32_t mesh_count;
		uint32_t transform_idx;
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

	struct DimBuffers
	{
		std::vector<std::unique_ptr<Buffer>> transform;
		std::vector<std::unique_ptr<Buffer>> vertex;
		std::unique_ptr<Buffer>				 entity;
		std::unique_ptr<Buffer>				 mesh;
	};
}
