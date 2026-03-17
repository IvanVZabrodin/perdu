include(FetchContent)

function(find_or_fetch NAME)
    cmake_parse_arguments(ARG "" "GIT_REPOSITORY;GIT_TAG" "CMAKE_ARGS" ${ARGN})

    find_package(${NAME} QUIET)
    if(NOT ${NAME}_FOUND)
        message(STATUS "${NAME} not found, fetching from ${ARG_GIT_REPOSITORY}...")
        include(FetchContent)
        FetchContent_Declare(${NAME}
            GIT_REPOSITORY ${ARG_GIT_REPOSITORY}
            GIT_TAG        ${ARG_GIT_TAG}
        )
        foreach(arg IN LISTS ARG_CMAKE_ARGS)
            string(REPLACE "=" ";" pair ${arg})
            list(GET pair 0 key)
            list(GET pair 1 val)
            set(${key} ${val} CACHE BOOL "" FORCE)
        endforeach()
        FetchContent_MakeAvailable(${NAME})
    else()
        message(STATUS "${NAME} found via find_package")
    endif()
endfunction()

find_or_fetch(SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        main
    CMAKE_ARGS     SDL_SHARED=OFF SDL_STATIC=ON SDL_TEST=OFF
)

find_or_fetch(EnTT
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG        v3.13.0
)

# find_or_fetch(spirv-cross
#     GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Cross.git
#     GIT_TAG        main
#     CMAKE_ARGS     SPIRV_CROSS_STATIC=ON
#                    SPIRV_CROSS_CLI=OFF
#                    SPIRV_CROSS_ENABLE_TESTS=OFF
# )
find_package(spirv_cross_core REQUIRED)

# find_or_fetch(shaderc
#     GIT_REPOSITORY https://github.com/google/shaderc.git
#     GIT_TAG        main
#     CMAKE_ARGS
#         SHADERC_SKIP_TESTS=ON
#         SHADERC_SKIP_EXAMPLES=ON
# 		SHADERC_SKIP_INSTALL=ON
# 		SHADERC_ENABLE_SHARED_CRT=OFF
#         SHADERC_SKIP_COPYRIGHT_CHECK=ON
# )
# find_package(libshaderc REQUIRED)
find_library(SHADERC_COMBINED REQUIRED shaderc_combined)

if(PERDU_BUILD_TESTS)
	FetchContent_Declare( # Catch2 is weird
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.0
    )
    FetchContent_MakeAvailable(Catch2)
endif()
