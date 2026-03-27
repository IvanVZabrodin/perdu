#pragma once

#include "entt/entity/fwd.hpp"
#include "perdu/engine/scene.hpp"

#include <entt/entt.hpp>
#include <entt/meta/resolve.hpp>
#include <utility>

namespace perdu {
	class Entity {
	  public:
		template <typename T, typename... Args>
		T& add(Args&&... args) {
			return _registry.emplace<T>(_entity, std::forward<Args>(args)...);
		}

		bool valid() const { return _entity != entt::null; }

		template <typename T>
		T& get() {
			return _registry.get<T>(_entity);
		}

		template <typename T>
		const T& get() const {
			return _registry.get<T>(_entity);
		}

		template <typename... T>
		bool has() const {
			return _registry.all_of<T...>(_entity);
		}

		entt::entity handle() const { return _entity; }

	  private:
		friend class Scene;

		entt::entity	_entity;
		entt::registry& _registry;

		Entity(entt::registry& reg, entt::entity e) :
			_entity(e), _registry(reg) {}
	};
}
