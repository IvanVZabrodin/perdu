#pragma once

#include "perdu/assets/asset_cache.hpp"

namespace perdu {

	struct PipelineInfo
	{
		ShaderHandle		 vert;
		ShaderHandle		 frag;
		SDL_GPUTextureFormat colour_format;
		PrimitiveType		 primitive_type = PrimitiveType::Triangles;
		bool				 depth_test		= true;
		bool				 depth_write	= true;
		bool				 cull_back		= true;
	};
}
