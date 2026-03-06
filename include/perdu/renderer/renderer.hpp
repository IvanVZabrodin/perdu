#pragma once

#include "perdu/renderer/gpu_context.hpp"

#include <cstdint>

namespace perdu {
	class Renderer {
	  public:
		explicit Renderer(GPUContext& ctx);
		~Renderer();

		Renderer(const Renderer&)			 = delete;
		Renderer& operator=(const Renderer&) = delete;

		void on_resize(uint32_t width, uint32_t height);
		void begin_frame();
		void end_frame();

	  private:
		GPUContext& _ctx;

		void create_depth_texture();
		void destroy_depth_texture();
	};
}
