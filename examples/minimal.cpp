#include "perdu/app/application.hpp"
#include "perdu/app/input.hpp"
#include "perdu/app/window.hpp"
#include "perdu/assets/assets.hpp"
#include "perdu/assets/resource_cache.hpp"
#include "perdu/components/camera.hpp"
#include "perdu/components/mesh.hpp"
#include "perdu/components/transform.hpp"
#include "perdu/core/log.hpp"
#include "perdu/core/maths.hpp"
#include "perdu/ecs/auto_system.hpp"
#include "perdu/engine/entity.hpp"
#include "perdu/engine/scene.hpp"
#include "perdu/renderer/mesh.hpp"
#include "perdu/renderer/shader.hpp"

#include <numbers>
#include <string>
#include <type_traits>

namespace events = perdu::events;

template <typename E>
	requires std::is_enum_v<E>
constexpr E next_enum(E e, std::underlying_type_t<E> count) {
	using U = std::underlying_type_t<E>;
	return static_cast<E>((static_cast<U>(e) + 1) % static_cast<U>(count));
}

std::string primitive_to_string(perdu::PrimitiveType p) {
	switch (p) {
		case perdu::PrimitiveType::Points	: return "points";
		case perdu::PrimitiveType::Lines	: return "lines";
		case perdu::PrimitiveType::Triangles: return "triangles";
	}
}

constexpr int mdim = 3;

class MyApp : public perdu::Application {
	void on_start() {
		auto frag = scene.assets.shaders.store(
		  "triangle.frag",
		  perdu::load_shader(gpu,
							 { perdu::asset_path("shaders/triangle.frag.spv"),
							   perdu::ShaderStage::Fragment }),
		  true);

		auto vert = scene.assets.shaders.store(
		  "triangle.vert",
		  perdu::load_shader(gpu,
							 {
								 perdu::asset_path("shaders/triangle.vert.spv"),
								 perdu::ShaderStage::Vertex,
							 }),
		  true);

		renderer.frag	  = frag;
		renderer.vert	  = vert;
		perdu::Entity cam = scene.vars.set("cam", scene.create());
		auto&		  k
		  = cam.add<perdu::Transform>(perdu::Vectorf{ 0.0, 0.0 }.extend(mdim));
		cam.add<perdu::Camera>(90.0f);

		view.camera = cam.handle();
		view.target = &window.wtx();

		perdu::Entity e = scene.vars.set("testentity", scene.create());
		auto&		  m = e.add<perdu::Mesh>(
		  perdu::make_cube(mdim, 1.0f, perdu::PrimitiveType::Triangles));
		e.add<perdu::Transform>(
		  perdu::Vectorf{ 0.0f, 0.0f, -3.0f }.extend(mdim));
		auto h
		  = scene.assets.meshes.store("hi", perdu::load_mesh(gpu, m), true);
		m.handle = h;
		m.recompute();
		scene.vars.set<size_t>("cpos", 0);
		scene.vars.set<size_t>("crot", 0);
		scene.vars.set<size_t>("tmin", 1);

		input.queue().subscribe<events::KeyEvent>([&](events::KeyEvent ev) {
			if (ev.action == events::PressAction::released) return;
			switch (ev.key) {
				case perdu::Key::A:
					{
						auto& r = scene.vars.get<size_t>("cpos");
						r		= perdu::pmod(r - 1, (size_t) m.dim);
						PERDU_LOG_DEBUG("new cpos: " + std::to_string(r));
						break;
					}
				case perdu::Key::D:
					{
						auto& r = scene.vars.get<size_t>("cpos");
						r		= perdu::pmod(r + 1, (size_t) m.dim);
						PERDU_LOG_DEBUG("new cpos: " + std::to_string(r));
						break;
					}
				case perdu::Key::Left:
					{
						auto& r = scene.vars.get<size_t>("crot");
						r = perdu::pmod(r - 1,
										(size_t) perdu::rotation_planes(m.dim));
						PERDU_LOG_DEBUG("new crot: " + std::to_string(r));
						break;
					}
				case perdu::Key::Right:
					{
						auto& r = scene.vars.get<size_t>("crot");
						r = perdu::pmod(r + 1,
										(size_t) perdu::rotation_planes(m.dim));
						PERDU_LOG_DEBUG("new crot: " + std::to_string(r));
						break;
					}
				case perdu::Key::Space:
					{
						scene.registry.view<perdu::Mesh>().each(
						  [&](perdu::Mesh& me) {
							  me.indices = perdu::make_cube(
											 me.dim,
											 1.0f,
											 next_enum(me.primitive_type, 3))
											 .indices;
							  PERDU_LOG_DEBUG(
								std::to_string(me.indices.size()));
							  me.primitive_type
								= next_enum(me.primitive_type, 3);
							  me.recompute();
						  });
						PERDU_LOG_DEBUG(
						  "primitive_type: "
						  + primitive_to_string(m.primitive_type));
						break;
					}
				case perdu::Key::Enter:
					{
						auto& mk = scene.vars.get<perdu::Entity>("testentity");
						perdu::PrimitiveType pt
						  = mk.get<perdu::Mesh>().primitive_type;
						auto  en = scene.create();
						auto& me = en.add<perdu::Mesh>(
						  perdu::make_cube(mdim, 1.0f, pt));
						size_t& tmin	 = scene.vars.get<size_t>("tmin");
						auto	l		 = perdu::load_mesh(gpu, me);
						l->cpu.debugname = "m" + std::to_string(tmin);
						auto h			 = scene.assets.meshes.store(
						  "m" + std::to_string(tmin), l, true);
						me.handle = h;
						float z	  = (float) (++tmin) * -3.0f;
						auto& tr  = en.add<perdu::Transform>(
						  perdu::Vectorf{ 0.0, 0.0, z }.extend(m.dim));
						break;
					}
				default: break;
			}
		});
	}
};

static constexpr float rotspeed	 = std::numbers::pi_v<float> / 2.0f;
static constexpr float movespeed = 2.0f;

static void inputtest(perdu::Scene& scene, float dt) {
	auto& state = scene.get_ctx<perdu::InputHandler::InputState>();
	auto& e		= scene.vars.get<perdu::Entity>("cam");
	// scene.registry.view<perdu::Mesh, perdu::Transform>().each(
	//   [&](perdu::Mesh& m, perdu::Transform& t) {
	// 	  if (state.is_key_down(perdu::Key::W)) {
	// 		  t.position[scene.vars.get<size_t>("cpos")] += movespeed * dt;
	// 	  }
	// 	  if (state.is_key_down(perdu::Key::S)) {
	// 		  t.position[scene.vars.get<size_t>("cpos")] -= movespeed * dt;
	// 	  }
	// 	  if (state.is_key_down(perdu::Key::Up)) {
	// 		  t.rotate_plane(scene.vars.get<size_t>("crot"), rotspeed * dt);
	// 	  }
	// 	  if (state.is_key_down(perdu::Key::Down)) {
	// 		  t.rotate_plane(scene.vars.get<size_t>("crot"), -rotspeed * dt);
	// 	  }
	// 	  if (state.is_key_down(perdu::Key::J)) {
	// 		  perdu::Mesh& m	= e.get<perdu::Mesh>();
	// 		  m.vertices[0][0] += dt;
	// 		  m.recompute();
	// 	  }
	//   });

	auto& t = e.get<perdu::Transform>();
	if (state.is_key_down(perdu::Key::W)) {
		t.position[scene.vars.get<size_t>("cpos")] += movespeed * dt;
	}
	if (state.is_key_down(perdu::Key::S)) {
		t.position[scene.vars.get<size_t>("cpos")] -= movespeed * dt;
	}
	if (state.is_key_down(perdu::Key::Up)) {
		t.rotate_plane(scene.vars.get<size_t>("crot"), rotspeed * dt);
	}
	if (state.is_key_down(perdu::Key::Down)) {
		t.rotate_plane(scene.vars.get<size_t>("crot"), -rotspeed * dt);
	}
}

static perdu::AutoSystem _inputtest{ inputtest, perdu::Phase::Input };

int main() {
	perdu::log::set_min_level(perdu::log::Level::Debug);
	// PERDU_LOG_DEBUG("debug message");
	// PERDU_LOG_INFO("info message");
	// PERDU_LOG_WARN("warn message");
	// PERDU_LOG_ERROR("error message");
	MyApp app{};
	app.set_target_fps(0);
	app.run("hi", 600, 600);
}
