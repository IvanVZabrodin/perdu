#pragma once

#include "perdu/core/maths.hpp"
namespace perdu {
	struct Transform
	{
		Vectorf position, rotation;

		size_t dim() const { return position.dim(); }

		Transform(const Vectorf& pos);
		Transform(size_t dim);

		Transform& rotate(const Vectorf& angles);
		Transform& rotate_plane(size_t axis, float theta);
	};
}
