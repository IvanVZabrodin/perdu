#pragma once

#include "perdu/core/event.hpp"
#include "perdu/renderer/gpu_context.hpp"

#include <cstdint>
#include <string_view>

namespace perdu {
	class Window {
	  public:
		Window(std::string_view title, uint32_t width, uint32_t height);
		~Window();

		Window(const Window&)			 = delete;
		Window& operator=(const Window&) = delete;

		GPUContext&		  ctx() { return _ctx; }
		const GPUContext& ctx() const { return _ctx; }
		const EventBus&	  events() const { return _events; }
		EventBus&		  events() { return _events; }

		bool should_close() const { return _should_close; }
		void poll_events();

	  private:
		EventBus   _events;
		GPUContext _ctx;
		bool	   _should_close = false;
	};

	namespace events {

		struct WindowQuit
		{};
		struct WindowResized
		{
			uint32_t width, height;
		};
	}
}
