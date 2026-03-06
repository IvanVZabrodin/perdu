add_library(perdu_components
	STATIC	mesh.cpp
)
add_library(perdu::components ALIAS perdu_components)

target_include_directories(perdu_components
	PUBLIC ${CMAKE_SOURCE_DIR}/include
	PRIVATE ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(perdu_components
	PUBLIC	perdu::core
			perdu::ecs
)
