#pragma once

#include "perdu/app/entity_desc.hpp"
#include "perdu/assets/asset_cache.hpp"

#include <entt/entt.hpp>

namespace perdu {
	class Entity;

	class Scene {
	  public:
		entt::registry registry;
		AssetCache	   assets;

		Entity create(const EntityDesc& desc);
		Entity create();

		void update(float dt);

		template <typename T, typename... Args>
		T& add_ctx(Args&&... args) {
			return registry.ctx().emplace<T>(std::forward<Args>(args)...);
		}

		template <typename T>
		T& get_ctx() {
			return registry.ctx().get<T>();
		}
	};
}
