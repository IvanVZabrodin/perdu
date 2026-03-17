#include "perdu/app/window.hpp"

#include "perdu/app/input.hpp"
#include "perdu/components/camera.hpp"
#include "perdu/core/assert.hpp"
#include "perdu/core/log.hpp"
#include "perdu/renderer/gpu_context.hpp"

#include <cstdint>
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <string_view>

namespace perdu {
	Window::Window(InputHandler& input, GPUContext& ctx) :
		_input(input), _wtx({ ctx }) {
		_input.queue().subscribe<events::WindowResized>(
		  [&](const auto& e) {
			  _wtx.width  = e.width;
			  _wtx.height = e.height;
			  _wtx.create_depth_texture();
		  },
		  0);

		_input.queue().subscribe<events::WindowQuit>(
		  [&](auto) { _should_close = true; }, 0);
	}

	void Window::open() {
		if (_title.empty()) PERDU_LOG_WARN("window title is not set");

		create(WindowFlags::Resizable);
	}

	void Window::open(std::string_view title, uint32_t width, uint32_t height) {
		_title		= title;
		_wtx.width	= width;
		_wtx.height = height;
		open();
	}

	void Window::close() {
		PERDU_ASSERT(_wtx.window, "trying to close non-existent window");
		SDL_ReleaseWindowFromGPUDevice(_wtx.ctx.device, _wtx.window);
		SDL_DestroyWindow(_wtx.window);
	}

	void Window::set_size(uint32_t width, uint32_t height) {
		_wtx.width	= width;
		_wtx.height = height;

		if (_wtx.window) SDL_SetWindowSize(_wtx.window, width, height);
	}

	void Window::set_title(std::string_view title) {
		_title = title;

		if (_wtx.window) SDL_SetWindowTitle(_wtx.window, title.data());
	}

	Window::~Window() {
		if (_wtx.window) close();
	}

	void Window::create(WindowFlags flags) {
		PERDU_ASSERT(_wtx.window == nullptr,
					 "trying to create window when it already exists");
		SDL_WindowFlags f = 0;
		if (flags & WindowFlags::Resizable) f |= SDL_WINDOW_RESIZABLE;
		if (flags & WindowFlags::Hidden) f |= SDL_WINDOW_HIDDEN;
		if (flags & WindowFlags::Borderless) f |= SDL_WINDOW_BORDERLESS;
		_wtx.window
		  = SDL_CreateWindow(_title.data(), _wtx.width, _wtx.height, f);
		SDL_ClaimWindowForGPUDevice(_wtx.ctx.device, _wtx.window);
		PERDU_ASSERT(_wtx.window, "window failed to initialise");

		SDL_SetGPUSwapchainParameters(_wtx.ctx.device,
									  _wtx.window,
									  SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
									  SDL_GPU_PRESENTMODE_IMMEDIATE // no vsync
		);
		_input.bus().emit(events::WindowResized{ _wtx.width, _wtx.height });
	}
}
