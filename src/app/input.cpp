#include "perdu/app/input.hpp"

#include "perdu/core/event.hpp"
#include "SDL3/SDL_events.h"

#include <bitset>
#include <cstdint>
#include <entt/entt.hpp>
#include <type_traits>
#include <utility>

namespace perdu {
	InputHandler::InputHandler(entt::registry& reg) : _bus(), _queue(&_bus) {
		InputState& state = reg.ctx().emplace<InputState>();

		_bus.subscribe<events::KeyEvent>([&state](const events::KeyEvent& e) {
			state._keys.set(std::to_underlying(e.key),
							std::to_underlying(e.action));
		});

		_bus.subscribe<events::MouseEvent>(
		  [&state](const events::MouseEvent& e) {
			  state._mouse.set(std::to_underlying(e.button),
							   std::to_underlying(e.action));
		  });

		_bus.subscribe<events::MouseMoved>(
		  [&state](const events::MouseMoved& e) {
			  state.mouse.x	 = e.x;
			  state.mouse.y	 = e.y;
			  state.mouse.dx = e.dx;
			  state.mouse.dy = e.dy;
		  });
	}

	void InputHandler::poll() {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
				case SDL_EVENT_QUIT: _bus.emit(events::WindowQuit{}); break;
				case SDL_EVENT_WINDOW_RESIZED:
					_bus.emit(events::WindowResized{
						(uint32_t) e.window.data1, (uint32_t) e.window.data2 });
					break;
				case SDL_EVENT_KEY_DOWN:
					_bus.emit(
					  events::KeyEvent{ static_cast<Key>(e.key.scancode),
										events::PressAction::pressed });
					break;
				case SDL_EVENT_KEY_UP:
					_bus.emit(
					  events::KeyEvent{ static_cast<Key>(e.key.scancode),
										events::PressAction::released });
					break;
				case SDL_EVENT_MOUSE_BUTTON_DOWN:
					_bus.emit(events::MouseEvent{
						static_cast<MouseButton>(e.button.button),
						events::PressAction::pressed });
					break;
				case SDL_EVENT_MOUSE_BUTTON_UP:
					_bus.emit(events::MouseEvent{
						static_cast<MouseButton>(e.button.button),
						events::PressAction::released });
					break;
				case SDL_EVENT_MOUSE_MOTION:
					_bus.emit(events::MouseMoved{
						e.motion.x, e.motion.y, e.motion.xrel, e.motion.yrel });
					break;
			}
		}
	}
}
