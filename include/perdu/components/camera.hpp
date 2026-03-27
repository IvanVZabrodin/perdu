#pragma once

#include <cstdint>
namespace perdu {
	class Camera {
	  public:
		Camera(float fov = 90.0f, float focal = 2.0f);

		void set_fov(float fov) { _fov = fov; }

	  private:
		float _fov;
		float _focal;
	};
}
