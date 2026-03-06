#include "perdu/app/window.hpp"

#include "perdu/app/input.hpp"
#include "perdu/core/assert.hpp"
#include "perdu/core/log.hpp"

#include <cstdint>
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>
#include <string_view>

namespace perdu {
	Window::Window(std::string_view title, uint32_t height, uint32_t width) :
		_events() {
		SDL_Init(SDL_INIT_VIDEO);

		_ctx.width	= width;
		_ctx.height = height;
		_ctx.window
		  = SDL_CreateWindow(title.data(), width, height, SDL_WINDOW_RESIZABLE);
		PERDU_ASSERT(_ctx.window, "window failed to initialise");

		_ctx.device
		  = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, nullptr);
		PERDU_ASSERT(_ctx.device, "gpu device failed to initialise");
		PERDU_LOG_DEBUG("using device driver "
						+ std::string(SDL_GetGPUDeviceDriver(_ctx.device)));

		SDL_ClaimWindowForGPUDevice(_ctx.device, _ctx.window);
	}

	void Window::poll_events() {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
				case SDL_EVENT_QUIT:
					_should_close = true;
					_events.emit(events::WindowQuit{});
					break;
				case SDL_EVENT_WINDOW_RESIZED:
					_ctx.width	= (uint32_t) e.window.data1;
					_ctx.height = (uint32_t) e.window.data2;
					_events.emit(
					  events::WindowResized{ _ctx.width, _ctx.height });
					break;
				case SDL_EVENT_KEY_DOWN:
					_events.emit(
					  events::KeyEvent{ static_cast<Key>(e.key.scancode),
										events::PressAction::pressed });
					break;
				case SDL_EVENT_KEY_UP:
					_events.emit(
					  events::KeyEvent{ static_cast<Key>(e.key.scancode),
										events::PressAction::released });
					break;
				case SDL_EVENT_MOUSE_BUTTON_DOWN:
					_events.emit(events::MouseEvent{
						static_cast<MouseButton>(e.button.button),
						events::PressAction::pressed });
					break;
				case SDL_EVENT_MOUSE_BUTTON_UP:
					_events.emit(events::MouseEvent{
						static_cast<MouseButton>(e.button.button),
						events::PressAction::released });
					break;
				case SDL_EVENT_MOUSE_MOTION:
					_events.emit(events::MouseMoved{
						e.motion.x, e.motion.y, e.motion.xrel, e.motion.yrel });
					break;
			}
		}
	}

	Window::~Window() {
		SDL_DestroyGPUDevice(_ctx.device);
		SDL_DestroyWindow(_ctx.window);
		SDL_Quit();
	}
}
