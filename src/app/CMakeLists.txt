add_library(perdu_app
	STATIC	scene.cpp
			window.cpp
			input.cpp
			application.cpp
)
add_library(perdu::app ALIAS perdu_app)

target_include_directories(perdu_app
	PUBLIC ${CMAKE_SOURCE_DIR}/include
	PRIVATE ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(perdu_app
	PUBLIC	perdu::core
			perdu::ecs
			perdu::renderer
	PRIVATE SDL3::SDL3
)
