#include "perdu/app/application.hpp"

#include "perdu/app/input.hpp"
#include "perdu/app/window.hpp"
#include "perdu/core/clock.hpp"
#include "perdu/core/log.hpp"
#include "perdu/engine/entity.hpp"
#include "perdu/renderer/gpu_context.hpp"

#include <chrono>
#include <cstdint>
#include <thread>

namespace perdu {
	void Application::run(std::string_view title,
						  uint32_t		   width,
						  uint32_t		   height) {
		window.open(title, width, height);
		scene.add_ctx<GPUContext*>(&gpu);
		scene.add_ctx<InputHandler*>(&input);

		scene.add_ctx<EventBus>();

		PERDU_LOG_INFO("start hook");
		on_start();

		renderer.view = &view;
		input.bus().subscribe<events::WindowResized>(
		  [&](const auto& ev) { renderer.on_resize(ev.width, ev.height); });
		input.register_winexposed_handler([this]() { do_frame(); });
		PERDU_LOG_INFO("starting mainloop");

		scene.add_ctx<Clock*>(&clock);
		while (!window.should_close()) do_frame();

		PERDU_LOG_INFO("stop hook");
		on_stop();
	}

	void Application::do_frame() {
		input.poll();
		input.queue().digest();
		scene.update(perdu::Phase::Input, debug_dt);

		scene.update(perdu::Phase::PreUpdate, debug_dt);
		scene.update(perdu::Phase::Update, debug_dt);
		scene.update(perdu::Phase::PostUpdate, debug_dt);

		renderer.prerender();
		renderer.begin_frame();
		scene.update(perdu::Phase::PreRender, debug_dt);
		scene.update(perdu::Phase::Render, debug_dt);
		renderer.render();
		scene.update(perdu::Phase::UI, debug_dt);
		renderer.end_frame();
		scene.update(perdu::Phase::PostRender, debug_dt);

		debug_dt = clock.tick();
		if (target_dt != 0.0f) {
			if (debug_dt < target_dt)
				std::this_thread::sleep_for(
				  std::chrono::duration<double>(target_dt - debug_dt));
		}
		debug_frame++;
		debug_dsum += debug_dt;

		if (debug_frame % 60 == 0) {
			double fps = 1.0f / (debug_dsum / 60.0f);
			PERDU_LOG_INFO("FPS: " + std::to_string(fps));
			scene.vars.set("fps", fps);
			debug_dsum = 0.0f;
		}
	}

	void Application::set_target_fps(uint32_t target) {
		target_dt = (target == 0 ? 0.0f : 1.0f / (float) target);
	}
}
