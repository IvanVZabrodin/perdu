#pragma once

#include "perdu/ecs/phase.hpp"

#include <entt/entt.hpp>
#include <functional>

namespace perdu {
	class SystemRegistry {
	  public:
		using SystemFn = std::function<void(entt::registry&, float)>;

		static SystemRegistry& get() {
			static SystemRegistry instance;
			return instance;
		}

		void register_system(SystemFn fn, Phase phase, int priority = 0);
		void update_all(entt::registry& reg, float dt);

	  private:
		struct Entry
		{
			Phase	 phase;
			int		 priority;
			SystemFn fn;
		};

		std::vector<Entry> _systems;
		bool			   _dirty = false;

		SystemRegistry() = default;
	};
}
