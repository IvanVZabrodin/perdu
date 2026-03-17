#pragma once

#include "perdu/app/input.hpp"
#include "perdu/components/camera.hpp"
#include "perdu/core/event.hpp"
#include "perdu/renderer/gpu_context.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace perdu {
	enum class WindowFlags : uint32_t { // TODO: add more, and implement
		None	   = 0,
		Resizable  = 1 << 0,
		Hidden	   = 1 << 1,
		Borderless = 1 << 2,
	};
	inline WindowFlags operator|(WindowFlags a, WindowFlags b) {
		return static_cast<WindowFlags>(static_cast<uint32_t>(a)
										| static_cast<uint32_t>(b));
	}
	inline bool operator&(WindowFlags a, WindowFlags b) {
		return static_cast<uint32_t>(a) & static_cast<uint32_t>(b);
	}


	class Window {
	  public:
		Window(InputHandler& input, GPUContext& ctx);
		~Window();

		Window(const Window&)			 = delete;
		Window& operator=(const Window&) = delete;

		void open();
		void open(std::string_view title, uint32_t width, uint32_t height);

		void close();

		void set_size(uint32_t width, uint32_t height);
		void set_title(std::string_view title);

		std::pair<uint32_t, uint32_t> get_size() const {
			return { _wtx.width, _wtx.height };
		}
		std::string_view get_title() const { return _title; }

		const WinTarget& wtx() const { return _wtx; }
		WinTarget&		 wtx() { return _wtx; }

		bool should_close() const { return _should_close; }
		void update_colour();

		void create(WindowFlags flags);

	  private:
		std::string	  _title;
		InputHandler& _input;
		WinTarget	  _wtx;
		bool		  _should_close = false;
	};
}
