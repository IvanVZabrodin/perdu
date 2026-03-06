#pragma once

#include "perdu/ecs/system_registry.hpp"

namespace perdu {
	struct AutoSystem
	{
		AutoSystem(SystemRegistry::SystemFn fn, Phase phase, int priority = 0) {
			SystemRegistry::get().register_system(
			  std::move(fn), phase, priority);
		}
	};
}
