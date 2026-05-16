#pragma once

#include "perdu/assets/asset_cache.hpp"

namespace perdu {
	struct Material
	{
		ShaderHandle vert;
		ShaderHandle frag;

		bool _dirty = true;
	};
}
