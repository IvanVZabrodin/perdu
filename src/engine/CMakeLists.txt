add_library(perdu_engine
	STATIC	asset_cache.cpp
			scene.cpp
)
add_library(perdu::engine ALIAS perdu_engine)

find_package(Vulkan REQUIRED)

target_include_directories(perdu_engine
	PUBLIC ${CMAKE_SOURCE_DIR}/include
	PRIVATE ${CMAKE_SOURCE_DIR}/src
			${Vulkan_INCLUDE_DIR}
)

target_link_libraries(perdu_engine
	PUBLIC	perdu::core
			perdu::ecs
			perdu::renderer
	PRIVATE Vulkan::Vulkan
)
