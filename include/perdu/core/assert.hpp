#pragma once

#include <cstdlib>

#if defined(_MSC_VER)
#	define PERDU_FUNCTION __FUNCSIG__
#else
#	define PERDU_FUNCTION __PRETTY_FUNCTION__
#endif

#ifdef NDEBUG
#	define PERDU_ASSERT(cond, msg) ((void) 0)
#else
#	ifdef PERDU_TESTING
#		include <string>
#		define PERDU_ASSERT(cond, msg)                                 \
			do {                                                        \
				if (!(cond)) {                                          \
					throw std::logic_error(std::string(msg)             \
										   + "\n condition:	" #cond     \
										   + "\n function: "            \
										   + PERDU_FUNCTION             \
										   + "\n file:		"               \
										   + __FILE__                   \
										   + ":"                        \
										   + std::to_string(__LINE__)); \
				}                                                       \
			} while (0)
#	else
#		include <iostream>
#		define PERDU_ASSERT(cond, msg)                                 \
			do {                                                        \
				if (!(cond)) {                                          \
					std::cerr                                           \
					  << "\033[91mPERDU_ASSERT\033[0m failed: \033[34m" \
					  << (msg)                                          \
					  << "\033[0m\n \033[35mcondition\033[0m:	" #cond \
					  << "\n \033[35mfunction\033[0m:	"                 \
					  << PERDU_FUNCTION                                 \
					  << "\n \033[35mfile\033[0m:		\033[32m"            \
					  << __FILE__                                       \
					  << "\033[0m:\033[36m"                             \
					  << __LINE__                                       \
					  << "\033[0m\n";                                   \
					std::abort();                                       \
				}                                                       \
			} while (0)
#	endif
#endif
