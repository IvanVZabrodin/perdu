add_library(perdu_renderer
	STATIC	pipeline.cpp
			renderer.cpp
)
add_library(perdu::renderer ALIAS perdu_renderer)

target_include_directories(perdu_renderer
	PUBLIC	${CMAKE_SOURCE_DIR}/include
	PRIVATE	${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(perdu_renderer
	PUBLIC	perdu::core
			perdu::ecs
	PRIVATE	SDL3::SDL3
)
