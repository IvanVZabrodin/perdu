#pragma once

#include <chrono>
namespace perdu {
	class Clock {
	  private:
		using clock_t	 = std::chrono::steady_clock;
		using time_point = clock_t::time_point;

		time_point _start, _last;

	  public:
		Clock();

		float tick();
		float elapsed() const;
	};
}
