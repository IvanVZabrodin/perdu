#include "perdu/app/scene.hpp"

#include "perdu/app/entity.hpp"
#include "perdu/ecs/system_registry.hpp"

#include <entt/entity/fwd.hpp>
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

	void Scene::update(float dt) {
		SystemRegistry::get().update_all(registry, dt);
	}
}
