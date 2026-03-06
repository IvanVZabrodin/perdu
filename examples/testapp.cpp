#include "perdu/app/application.hpp"
#include "perdu/app/entity.hpp"
#include "perdu/app/entity_desc.hpp"
#include "perdu/app/input.hpp"
#include "perdu/app/scene.hpp"
#include "perdu/assets/resource_cache.hpp"
#include "perdu/components/transform.hpp"
#include "perdu/core/clock.hpp"
#include "perdu/core/log.hpp"
#include "perdu/core/maths.hpp"
#include "perdu/ecs/auto_system.hpp"
#include "perdu/ecs/system_registry.hpp"

#include <string>

namespace events = perdu::events;

struct TestComponent
{
	int i = 0;
};

class MyApp : public perdu::Application {
	void on_start() {
		perdu::EntityDesc d = perdu::EntityDesc{}.with<perdu::Transform>(
		  perdu::Vectorf{ 1.0, 2.0, 3.0 });
		auto en = scene.create(d);

		window.events().subscribe<events::KeyEvent>(
		  [&en, this](const events::KeyEvent& e) {
			  if (e.action == events::PressAction::pressed
				  && e.key == perdu::Key::A)
			  {
				  auto&			t = en.get<perdu::Transform>();
				  perdu::Clock* c = scene.get_ctx<perdu::Clock*>();
				  t.position	 += { 0.0f, 0.0f, 1.0f };
				  if (t.position[1] == 1.0f) {
					  t.rotation[0] = c->elapsed();
					  t.position[1] = 0.0f;
				  }
				  t.rotation[1] = c->elapsed();
			  }
		  });
	}
};


static void testsystem(entt::registry& reg, float dt) {
	reg.view<perdu::Transform>().each([](perdu::Transform& t) {
		if ((int) (t.position[2]) % 12 == 0) {
			t.position[1] = 1.0f;
		} else if ((int) (t.position[2] - 1.0f) % 12 == 0) {
			float dtime = (t.rotation[1] - t.rotation[0]) / 12.0f;
			PERDU_LOG_DEBUG("Average frame time (over 12 frames): "
							+ std::to_string(dtime));
		}
	});
}

static perdu::AutoSystem _test{ testsystem, perdu::Phase::Render };


int main() {
	perdu::log::set_min_level(perdu::log::Level::Debug);
	MyApp app{};
	app.run();
}
