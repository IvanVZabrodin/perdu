#include "perdu/components/transform.hpp"

#include "perdu/core/assert.hpp"
#include "perdu/core/maths.hpp"

#include <numbers>

static constexpr float fulldeg = std::numbers::pi_v<float> * 2;

namespace perdu {
	Transform::Transform(const Vectorf& pos) :
		position(pos), rotation(rotation_planes(pos.dim()), 0.0f) {};
	Transform::Transform(size_t dim) :
		position(dim, 0.0f), rotation(rotation_planes(dim), 0.0f) {}

	Transform& Transform::rotate(const Vectorf& angles) {
		PERDU_ASSERT(angles.dim() == rotation.dim(),
					 "angles must have same dim as rotation");
		for (size_t i = 0; i < rotation.dim(); i++) {
			rotation[i] = pmod(rotation[i] + angles[i], fulldeg);
		}

		return *this;
	}

	Transform& Transform::rotate_plane(size_t axis, float theta) {
		PERDU_ASSERT(axis < rotation_planes(dim()), "invalid axis");
		rotation[axis] = pmod(rotation[axis] + theta, fulldeg);
		return *this;
	}
}
