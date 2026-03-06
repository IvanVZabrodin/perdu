#pragma once

#include "perdu/core/maths.hpp"
namespace perdu {
	struct Transform
	{
		Vectorf position, rotation;

		size_t dim() const { return position.dim(); }
	};
}
