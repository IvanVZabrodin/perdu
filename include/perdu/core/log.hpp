#pragma once

#include <functional>
#include <string>
#include <string_view>

#ifdef NDEBUG
#	define PERDU_LOG_DEBUG(msg) ((void) 0)
#	define PERDU_MARKER()		 ((void) 0)
#else
#	define PERDU_LOG_DEBUG(msg) \
		::perdu::log::log(::perdu::log::Level::Debug, msg, __FILE__, __LINE__)
#	define PERDU_MARKER()                                              \
		::perdu::log::log(::perdu::log::Level::Debug,                   \
						  "marker at line " + std::to_string(__LINE__), \
						  __FILE__,                                     \
						  __LINE__)
#endif
#define PERDU_LOG_INFO(msg) \
	::perdu::log::log(::perdu::log::Level::Info, msg, __FILE__, __LINE__)
#define PERDU_LOG_WARN(msg) \
	::perdu::log::log(::perdu::log::Level::Warn, msg, __FILE__, __LINE__)
#define PERDU_LOG_ERROR(msg) \
	::perdu::log::log(::perdu::log::Level::Error, msg, __FILE__, __LINE__)

namespace perdu::log {
	enum class Level { Debug, Info, Warn, Error };
	enum class Colour { Red, Green, Blue, Yellow, Grey, White, Black, Cyan };

	std::string coloured(std::string str, Colour colour);

	std::string level_str(Level level);

	Colour level_colour(Level level);

	inline constexpr std::string coloured_level(Level level) {
		return coloured(level_str(level), level_colour(level));
	}

	using Sink = std::function<void(Level, std::string_view, const char*, int)>;

	void log(Level level, std::string_view msg, const char* file, int line);

	void default_sink(Level			   level,
					  std::string_view msg,
					  const char*	   file,
					  int			   line);

	void set_sink(Sink sink);
	void set_min_level(Level level);
}
