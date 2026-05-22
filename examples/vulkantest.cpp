#include "perdu/app/application.hpp"

class MyApp : public perdu::Application {
  public:
	MyApp() : perdu::Application() {};

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

		renderer.frag = frag;
		renderer.vert = vert;
	}
};
int main() {
	perdu::log::set_min_level(perdu::log::Level::Debug);
	MyApp m{};
	m.set_target_fps(0);
	m.run();
}
