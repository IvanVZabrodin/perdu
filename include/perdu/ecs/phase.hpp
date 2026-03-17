#pragma once

#include <cstddef>
namespace perdu {
	enum class Phase {
		Input	   = 0,
		PreUpdate  = 1,
		Update	   = 2,
		PostUpdate = 3,
		PreRender  = 4,
		Render	   = 5,
		UI		   = 6,
		PostRender = 7
	};

	static constexpr size_t phase_count = 8;
}
