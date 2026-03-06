add_executable(perdu_tests
	tests/maths.cpp
)

target_compile_definitions(perdu_tests PRIVATE PERDU_TESTING)

target_link_libraries(perdu_tests
	PRIVATE perdu::core
			Catch2::Catch2WithMain
)

add_executable(perdu_benchmarks
    benchmarks/maths.cpp
)
target_compile_definitions(perdu_benchmarks PRIVATE PERDU_TESTING)


target_link_libraries(perdu_benchmarks 
	PRIVATE perdu::core
			Catch2::Catch2WithMain
)

add_test(
    NAME benchmarks
    COMMAND perdu_benchmarks "[!benchmark]"
)
set_tests_properties(benchmarks PROPERTIES LABELS "benchmark")

include(CTest)
include(Catch)
catch_discover_tests(perdu_tests)
