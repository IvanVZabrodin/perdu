#include "perdu/ecs/system_registry.hpp"

#include "perdu/core/log.hpp"

#include <algorithm>
#include <string>

std::string phase_str(perdu::Phase phase) {
	switch (phase) {
		case perdu::Phase::Input	 : return "Input";
		case perdu::Phase::PreUpdate : return "PreUpdate";
		case perdu::Phase::Update	 : return "Update";
		case perdu::Phase::PostUpdate: return "PostUpdate";
		case perdu::Phase::PreRender : return "PreRender";
		case perdu::Phase::Render	 : return "Render";
		case perdu::Phase::UI		 : return "UI";
		case perdu::Phase::PostRender: return "PostRender";
	}
	return "????";
}

namespace perdu {
	void
	  SystemRegistry::register_system(SystemFn fn, Phase phase, int priority) {
		PERDU_LOG_INFO("System registered for "
					   + phase_str(phase)
					   + ":"
					   + std::to_string(priority));
		_systems.push_back({ phase, priority, std::move(fn) });
		_dirty = true;
	}

	void SystemRegistry::update_all(entt::registry& reg, float dt) {
		if (_dirty) {
			std::stable_sort(_systems.begin(),
							 _systems.end(),
							 [](const Entry& a, const Entry& b) {
								 if (a.phase != b.phase)
									 return a.phase < b.phase;
								 return a.priority < b.priority;
							 });
			_dirty = false;
		}
		for (auto& e : _systems) e.fn(reg, dt);
	}
}
