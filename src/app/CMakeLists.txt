add_library(perdu_app
	STATIC	
	# window.cpp
			input.cpp
			application.cpp
)
add_library(perdu::app ALIAS perdu_app)

find_package(Vulkan REQUIRED)
target_compile_definitions(perdu_renderer PUBLIC
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
        VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
)


target_include_directories(perdu_app
	PUBLIC ${CMAKE_SOURCE_DIR}/include
	PRIVATE ${CMAKE_SOURCE_DIR}/src
			${Vulkan_INCLUDE_DIR}
)

target_link_libraries(perdu_app
	PUBLIC	perdu::core
			perdu::ecs
			perdu::engine
			perdu::renderer
	PRIVATE SDL3::SDL3
			Vulkan::Vulkan
)
