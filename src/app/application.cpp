#include "perdu/app/application.hpp"

#include "perdu/app/window.hpp"
#include "perdu/core/clock.hpp"
#include "perdu/core/log.hpp"
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

		scene.add_ctx<EventBus>();
		EventQueue& inputqueue = input.queue();
		renderer.target		   = &window.wtx();


		PERDU_LOG_INFO("start hook");
		on_start();

		PERDU_LOG_INFO("starting mainloop");

		uint32_t frame = 0;
		float	 dsum  = 0.0f;

		Clock& clock = scene.add_ctx<Clock>();
		float  dt	 = 0.0f;
		while (!window.should_close()) {
			input.poll();
			inputqueue.digest();
			scene.update(perdu::Phase::Input, dt);

			scene.update(perdu::Phase::PreUpdate, dt);
			scene.update(perdu::Phase::Update, dt);
			scene.update(perdu::Phase::PostUpdate, dt);

			renderer.begin_frame();
			scene.update(perdu::Phase::PreRender, dt);
			renderer.prerender();
			scene.update(perdu::Phase::Render, dt);
			renderer.render();
			scene.update(perdu::Phase::UI, dt);
			renderer.end_frame();
			scene.update(perdu::Phase::PostRender, dt);

			dt = clock.tick();
			if (target_dt != 0.0f) {
				PERDU_LOG_DEBUG("sleeping");
				if (dt < target_dt)
					std::this_thread::sleep_for(
					  std::chrono::duration<float>(target_dt - dt));
			}
			frame++;
			dsum += dt;

			if (frame % 24 == 0) {
				float fps = 1.0f / (dsum / 24.0f);
				PERDU_LOG_DEBUG("FPS: " + std::to_string(fps));
				dsum = 0.0f;
			}
		}

		PERDU_LOG_INFO("stop hook");
		on_stop();
	}

	void Application::set_target_fps(uint32_t target) {
		target_dt = (target == 0 ? 0.0f : 1.0f / (float) target);
	}
}
