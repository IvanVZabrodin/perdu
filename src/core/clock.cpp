#include "perdu/core/clock.hpp"

#include <chrono>

namespace perdu {
	Clock::Clock() : _start(clock_t::now()), _last(clock_t::now()) {};

	float Clock::tick() {
		auto  now = clock_t::now();
		float dt  = std::chrono::duration<float>(now - _last).count();
		_last	  = now;
		return dt;
	}

	float Clock::elapsed() const {
		return std::chrono::duration<float>(clock_t::now() - _start).count();
	}
}
