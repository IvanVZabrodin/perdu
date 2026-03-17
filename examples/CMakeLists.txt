add_executable(perdu_example
	minimal.cpp
)

compile_shaders(perdu_example
	SOURCES	${CMAKE_SOURCE_DIR}/shaders/triangle.vert
			${CMAKE_SOURCE_DIR}/shaders/triangle.frag
	OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin/shaders
)

target_link_libraries(perdu_example
	PRIVATE	perdu::core
			perdu::ecs
			perdu::components
			perdu::app
)
