#include "perdu/core/log.hpp"

#include <iostream>
#include <string_view>

#ifdef _WIN32
#	include <io.h>
#	include <cstdio>
#else
#	include <unistd.h>
#endif

#ifdef _WIN32
static bool is_terminal = _isatty(_fileno(stderr));
#else
static bool is_terminal = isatty(STDERR_FILENO);
#endif

namespace perdu::log::detail {
	static Level min_level	  = Level::Info;
	static Sink	 current_sink = default_sink;
}

namespace perdu::log {
	void log(Level level, std::string_view msg, const char* file, int line) {
		if (level < detail::min_level) return;
		detail::current_sink(level, msg, file, line);
	}

	void set_sink(Sink sink) {
		detail::current_sink = sink;
	}

	void set_min_level(Level level) {
		detail::min_level = level;
	}

	void default_sink(Level			   level,
					  std::string_view msg,
					  const char*	   file,
					  int			   line) {
		if (is_terminal) {
			std::cerr
			  << "["
			  << coloured_level(level)
			  << "]\t"
			  << msg
			  << " ("
			  << coloured(std::string(file), Colour::Green)
			  << ":"
			  << coloured(std::to_string(line), Colour::Cyan)
			  << ")\n";
		} else {
			std::cerr
			  << "["
			  << level_str(level)
			  << "]\t"
			  << msg
			  << " ("
			  << std::string(file)
			  << ":"
			  << std::to_string(line)
			  << ")\n";
		}
	}

	std::string level_str(Level level) {
		switch (level) {
			case Level::Debug: return "debug";
			case Level::Info : return "info";
			case Level::Warn : return "warn";
			case Level::Error: return "error";
		}
		return "????";
	}

	perdu::log::Colour level_colour(perdu::log::Level level) {
		switch (level) {
			case perdu::log::Level::Debug: return perdu::log::Colour::Grey;
			case perdu::log::Level::Info : return perdu::log::Colour::Blue;
			case perdu::log::Level::Warn : return perdu::log::Colour::Yellow;
			case perdu::log::Level::Error: return perdu::log::Colour::Red;
		}
		return perdu::log::Colour::White;
	}


	std::string coloured(std::string str, Colour colour) {
		switch (colour) {
			case Colour::Red   : return "\033[31m" + str + "\033[0m";
			case Colour::Green : return "\033[32m" + str + "\033[0m";
			case Colour::Blue  : return "\033[34m" + str + "\033[0m";
			case Colour::Yellow: return "\033[33m" + str + "\033[0m";
			case Colour::Grey  : return "\033[90m" + str + "\033[0m";
			case Colour::White : return "\033[97m" + str + "\033[0m";
			case Colour::Black : return "\033[30m" + str + "\033[0m";
			case Colour::Cyan  : return "\033[36m" + str + "\033[0m";
		}
		return std::string("\033[0m") + str;
	}
}
