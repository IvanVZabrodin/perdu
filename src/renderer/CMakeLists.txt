add_library(perdu_renderer
	STATIC	pipeline.cpp
			pipeline_cache.cpp
			renderer.cpp
			shader.cpp
			shader_reflect.cpp
			mesh.cpp
			shader_gen.cpp
			compute.cpp
			renderview.cpp
			gpu_context.cpp
)
add_library(perdu::renderer ALIAS perdu_renderer)

target_include_directories(perdu_renderer
	PUBLIC	${CMAKE_SOURCE_DIR}/include
	PRIVATE	${CMAKE_SOURCE_DIR}/src
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
			SPIRV-tools
			SPIRV-Tools-opt
)
