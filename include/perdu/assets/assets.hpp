#pragma once

#include <optional>
namespace perdu {
	template <typename CPU, typename GPU = void>
	struct Asset
	{
		CPU				   cpu;
		std::optional<GPU> gpu;

		bool is_uploaded() const { return gpu.has_value(); }
	};
}
