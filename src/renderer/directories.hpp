#pragma once

#include <filesystem>
#include <SDL3/SDL_filesystem.h>
namespace perdu {
	const std::filesystem::path rootdir
	  = std::filesystem::path(SDL_GetBasePath()).parent_path();

}
