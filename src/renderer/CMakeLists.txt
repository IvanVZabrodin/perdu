add_library(perdu_renderer
	STATIC	pipeline.cpp
			# pipeline_cache.cpp
			renderer.cpp
			shader.cpp
			shader_reflect.cpp
			# mesh.cpp
			shader_gen.cpp
			# compute.cpp
			# renderview.cpp
			gpu_context.cpp
			# area_manager.cpp
)
add_library(perdu::renderer ALIAS perdu_renderer)

find_package(Vulkan REQUIRED)
message(STATUS "Vulkan include: ${Vulkan_INCLUDE_DIRS}")

target_compile_definitions(perdu_renderer PUBLIC
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
        VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
)

target_include_directories(perdu_renderer
	PUBLIC	${CMAKE_SOURCE_DIR}/include
	PRIVATE	${CMAKE_SOURCE_DIR}/src 
			${Vulkan_INCLUDE_DIR}
)

target_compile_options(perdu_renderer PRIVATE
    -Wno-missing-field-initializers
)

target_link_libraries(perdu_renderer
	PUBLIC	perdu::core
			perdu::ecs
			perdu::components
	PRIVATE	SDL3::SDL3
			spirv-cross-core
			shaderc_combined
			glslang
			SPIRV-Tools
			SPIRV-Tools-opt
			Vulkan::Vulkan
)
