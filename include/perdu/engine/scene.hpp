#pragma once

#include "perdu/assets/asset_cache.hpp"
#include "perdu/core/assert.hpp"
#include "perdu/ecs/phase.hpp"
#include "perdu/engine/entity_desc.hpp"

#include <any>
#include <cstdint>
#include <entt/entt.hpp>
#include <string>
#include <string_view>
#include <unordered_map>

namespace perdu {
	class Entity;

	class SceneVars {
	  public:
		template <typename T>
		T& set(std::string_view name, T value) {
			_vars[hash(name)] = std::move(value);
			return std::any_cast<T&>(_vars[hash(name)]);
		}

		template <typename T>
		T& try_set(std::string_view name, T value) { // TODO: Make efficient
			auto it = _vars.find(hash(name));
			if (it == _vars.end()) return std::any_cast<T&>(it->second);
			_vars[hash(name)] = std::move(value);
			return std::any_cast<T&>(_vars[hash(name)]);
		}

		template <typename T>
		T& get(std::string_view name) {
			auto it = _vars.find(hash(name));
			PERDU_ASSERT(it != _vars.end(),
						 "failed to find scene var of name "
						   + std::string(name));
			return std::any_cast<T&>(it->second);
		}

		template <typename T>
		T* try_get(std::string_view name) {
			auto it = _vars.find(hash(name));
			if (it == _vars.end()) return nullptr;
			return std::any_cast<T>(&it->second);
		}

		bool has(std::string_view name) { return _vars.contains(hash(name)); }

		void remove(std::string_view name) { _vars.erase(hash(name)); }

	  private:
		std::unordered_map<uint32_t, std::any> _vars;
	};

	class Scene {
	  public:
		entt::registry registry;
		AssetCache	   assets;
		SceneVars	   vars;

		Entity create(const EntityDesc& desc);
		Entity create();

		void update(Phase phase, float dt);

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
