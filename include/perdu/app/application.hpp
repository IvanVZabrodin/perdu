#pragma once

#include "perdu/app/input.hpp"
#include "perdu/app/window.hpp"
#include "perdu/engine/scene.hpp"
#include "perdu/renderer/renderer.hpp"

#include <cstdint>
#include <string_view>

namespace perdu {
	class Application {
	  public:
		virtual ~Application() = default;
		void run(std::string_view title	 = "perdu",
				 uint32_t		  width	 = 800,
				 uint32_t		  height = 600);

		void set_target_fps(uint32_t target);

	  protected:
		GPUContext	 gpu{};
		Scene		 scene{};
		Renderer	 renderer{ gpu, scene };
		InputHandler input{ scene.registry };
		Window		 window{ input, gpu };

		virtual void on_start() {}
		virtual void on_stop() {}

	  private:
		float target_dt = 1.0f / 60.0f;
	};
}
