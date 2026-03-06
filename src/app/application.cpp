#include "perdu/app/application.hpp"

#include "perdu/app/window.hpp"
#include "perdu/core/clock.hpp"
#include "perdu/core/log.hpp"
#include "perdu/renderer/gpu_context.hpp"

namespace perdu {
	void Application::run() {
		scene.add_ctx<GPUContext*>(&window.ctx());
		scene.add_ctx<Window*>(&window);

		scene.add_ctx<EventBus>();

		window.events().subscribe<events::WindowResized>(
		  [this](const events::WindowResized& e) {
			  renderer.on_resize(e.width, e.height);
		  });

		PERDU_LOG_INFO("Start hook");
		on_start();

		PERDU_LOG_INFO("Starting mainloop");
		Clock clock{};

		scene.add_ctx<Clock*>(&clock);
		while (!window.should_close()) {
			window.poll_events();
			scene.update(clock.tick());
		}

		PERDU_LOG_INFO("Stop hook");
		on_stop();
	}
}
