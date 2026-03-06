add_executable(perdu_example
	testapp.cpp
)

target_link_libraries(perdu_example
	PRIVATE	perdu::core
			perdu::ecs
			perdu::components
			perdu::app
)
