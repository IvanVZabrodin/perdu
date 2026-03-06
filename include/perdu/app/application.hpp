#pragma once

#include "perdu/app/input.hpp"
#include "perdu/app/scene.hpp"
#include "perdu/app/window.hpp"
#include "perdu/renderer/renderer.hpp"

#include <string_view>

namespace perdu {
	class Application {
	  public:
		virtual ~Application() = default;
		void run();

	  protected:
		Window		 window{ "Perdu", 800, 600 };
		Renderer	 renderer{ window.ctx() };
		Scene		 scene{};
		InputHandler input{ window.events(), scene.registry };

		virtual void on_start() {}
		virtual void on_stop() {}
	};
}
