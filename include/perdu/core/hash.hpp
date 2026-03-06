#pragma once
#include <cstdint>
#include <string_view>

namespace perdu {
	constexpr uint32_t hash(std::string_view str) {
		uint32_t h = 2'166'136'261u;
		for (char c : str) {
			h ^= static_cast<uint32_t>(c);
			h *= 16'777'619u;
		}
		return h;
	}

	namespace literals {
		constexpr uint32_t operator""_h(const char* str, size_t len) {
			return hash(std::string_view(str, len));
		}
	}
}
