#include "perdu/app/input.hpp"

#include "perdu/core/event.hpp"

#include <bitset>
#include <cstdint>
#include <entt/entt.hpp>
#include <type_traits>
#include <utility>

namespace perdu {
	InputHandler::InputHandler(EventBus& bus, entt::registry& reg) {
		InputState& state = reg.ctx().emplace<InputState>();

		bus.subscribe<events::KeyEvent>([&state](const events::KeyEvent& e) {
			state._keys.set(std::to_underlying(e.key),
							std::to_underlying(e.action));
		});

		bus.subscribe<events::MouseEvent>(
		  [&state](const events::MouseEvent& e) {
			  state._mouse.set(std::to_underlying(e.button),
							   std::to_underlying(e.action));
		  });

		bus.subscribe<events::MouseMoved>(
		  [&state](const events::MouseMoved& e) {
			  state.mouse.x	 = e.x;
			  state.mouse.y	 = e.y;
			  state.mouse.dx = e.dx;
			  state.mouse.dy = e.dy;
		  });
	}
}
