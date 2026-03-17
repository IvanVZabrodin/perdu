function(compile_shaders TARGET)
	find_program(GLSLC glslc REQUIRED)

	cmake_parse_arguments(ARG "" "OUTPUT_DIR" "SOURCES" ${ARGN})
	
	if(NOT ARG_OUTPUT_DIR)
		set(ARG_OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders)
	endif()

	file(MAKE_DIRECTORY ${ARG_OUTPUTDIR})

	set(SPIRV_FILES "")

	foreach(SHADER ${ARG_SOURCES})
		get_filename_component(SHADER_NAME ${SHADER} NAME)
		set(SPIRV "${ARG_OUTPUT_DIR}/${SHADER_NAME}.spv")

		add_custom_command(
			OUTPUT ${SPIRV}
			COMMAND ${GLSLC} ${SHADER} -o ${SPIRV}
			DEPENDS ${SHADER}
			COMMENT "Compiling shader ${SHADER_NAME}"
		)

		list(APPEND SPIRV_FILES ${SPIRV})
	endforeach()

	add_custom_target(${TARGET}_shaders ALL DEPENDS ${SPIRV_FILES})
	add_dependencies(${TARGET} ${TARGET}_shaders)
endfunction()
