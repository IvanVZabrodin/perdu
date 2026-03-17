#pragma once

#include "perdu/ecs/phase.hpp"

#include <entt/entt.hpp>
#include <functional>

namespace perdu {
	class Scene;

	class SystemRegistry {
	  public:
		using SystemFn = std::function<void(Scene&, float)>;

		static SystemRegistry& get() {
			static SystemRegistry instance;
			return instance;
		}

		void register_system(SystemFn fn, Phase phase, int priority = 0);
		void update_all(entt::registry& reg, float dt);

		void update_phase(Phase phase, Scene& reg, float dt);

	  private:
		struct Entry
		{
			// Phase	 phase;
			int		 priority;
			SystemFn fn;
		};

		std::array<std::vector<Entry>, phase_count> _phases;
		bool										_dirty = false;

		void sort();

		SystemRegistry() = default;
	};
}
