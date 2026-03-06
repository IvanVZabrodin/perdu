#pragma once

#include <entt/entt.hpp>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
namespace perdu {
	class EntityDesc {
	  public:
		explicit constexpr EntityDesc() = default;


		template <typename T, typename... Args>
		EntityDesc& with(Args&&... args) {
			_appliers.push_back([args = std::make_tuple(std::decay_t<Args>(
								   std::forward<Args>(args))...)](
								  entt::registry& reg, entt::entity e) {
				std::apply(
				  [&](auto&&... a) mutable {
					  reg.emplace<T>(e, std::forward<decltype(a)>(a)...);
				  },
				  args);
			});
			return *this;
		}

		template <typename T, typename... Args>
		EntityDesc add(Args&&... args) {
			EntityDesc copy = *this;
			return copy.with<T>(std::forward<Args>(args)...);
		}

		void apply(entt::registry& reg, entt::entity e) const {
			for (auto& a : _appliers) a(reg, e);
		}

	  private:
		using Applier = std::function<void(entt::registry&, entt::entity)>;
		std::vector<Applier> _appliers{};
	};
}
