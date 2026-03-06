add_library(perdu_ecs
	STATIC	system_registry.cpp
)
add_library(perdu::ecs ALIAS perdu_ecs)

target_include_directories(perdu_ecs
	PUBLIC ${CMAKE_SOURCE_DIR}/include
	PRIVATE ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(perdu_ecs
	PUBLIC	perdu::core
			EnTT::EnTT
)
