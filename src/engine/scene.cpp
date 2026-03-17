#include "perdu/engine/scene.hpp"

#include "perdu/ecs/phase.hpp"
#include "perdu/ecs/system_registry.hpp"
#include "perdu/engine/entity.hpp"

#include <entt/entt.hpp>

namespace perdu {
	Entity Scene::create(const EntityDesc& desc) {
		entt::entity e = registry.create();
		Entity		 ent(registry, e);
		desc.apply(registry, ent._entity);
		return ent;
	};

	Entity Scene::create() {
		entt::entity e = registry.create();
		Entity		 ent(registry, e);
		return ent;
	};

	void Scene::update(Phase phase, float dt) {
		SystemRegistry::get().update_phase(phase, *this, dt);
	}
}
