add_library(perdu_core
	STATIC	log.cpp
			clock.cpp
)
add_library(perdu::core ALIAS perdu_core)

target_include_directories(perdu_core
	PUBLIC ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(perdu_core
	PUBLIC	perdu::compiler_options
)
