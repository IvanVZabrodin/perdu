#include "perdu/app/application.hpp"
#include "perdu/app/input.hpp"
#include "perdu/app/window.hpp"
#include "perdu/assets/assets.hpp"
#include "perdu/assets/resource_cache.hpp"
#include "perdu/components/transform.hpp"
#include "perdu/core/clock.hpp"
#include "perdu/core/log.hpp"
#include "perdu/core/maths.hpp"
#include "perdu/ecs/auto_system.hpp"
#include "perdu/ecs/phase.hpp"
#include "perdu/ecs/system_registry.hpp"
#include "perdu/engine/entity.hpp"
#include "perdu/engine/entity_desc.hpp"
#include "perdu/engine/scene.hpp"
#include "perdu/renderer/shader.hpp"

#include <cstdint>
#include <string>

namespace events = perdu::events;

struct TestComponent
{
	int i = 0;
};

class MyApp : public perdu::Application {
	void on_start() {
		target_fps			= 120; // Unlimited
		perdu::EntityDesc d = perdu::EntityDesc{}.with<perdu::Transform>(
		  perdu::Vectorf{ 1.0, 2.0, 3.0 });
		auto en = scene.create(d);

		auto frag = scene.assets.shaders.store(
		  "triangle.frag",
		  perdu::load_shader(
			window.ctx(),
			{ "shaders/triangle.frag.spv", perdu::ShaderStage::Fragment }));

		auto vert = scene.assets.shaders.store(
		  "triangle.vert",
		  perdu::load_shader(window.ctx(),
							 {
								 "shaders/triangle.vert.spv",
								 perdu::ShaderStage::Vertex,
							 }));

		renderer.create_pipeline({ vert, frag });


		window.events().subscribe<events::KeyEvent>(
		  [en, this](const events::KeyEvent& e) mutable {
			  if (e.action == events::PressAction::pressed
				  && e.key == perdu::Key::A)
			  {
				  en.get<perdu::Transform>().position[0] += 1.0f;
			  }
		  });
	}
};


static void testsystem(perdu::Scene& scene, float dt) {
	perdu::Clock& c = scene.get_ctx<perdu::Clock>();
	scene.registry.view<perdu::Transform>().each([&](perdu::Transform& t) {
		if ((int) t.position[0] % 12 == 0) {
			double ft = (c.elapsed() - t.position[1]) / 12;
			PERDU_LOG_DEBUG("Frame time "
							+ std::to_string(ft)
							+ ". aka "
							+ std::to_string(1 / ft)
							+ "fps");
			t.position[1] = c.elapsed();
		}
		t.position[0] += 1.0f;
	});
}

static void inputtest(perdu::Scene& scene, float dt) {
	auto& state = scene.get_ctx<perdu::InputHandler::InputState>();
	if (state.is_key_down(perdu::Key::A)) {
		perdu::Window* win = scene.get_ctx<perdu::Window*>();
		win->set_title("A is pressed: " + std::to_string(dt));
		auto s = win->get_size();
		win->set_size(s.first, s.second + 1);
	}
}

static perdu::AutoSystem _test{ testsystem, perdu::Phase::Render };
static perdu::AutoSystem _inputtest{ inputtest, perdu::Phase::Input };


int main() {
	perdu::log::set_min_level(perdu::log::Level::Debug);
	MyApp app{};
	app.run("hi");
}
