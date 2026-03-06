#include "perdu/core/maths.hpp"

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace perdu;

TEST_CASE("Vector benchmarks", "[!benchmark][vector]") {

	BENCHMARK("construct dim=4") {
		return Vectorf(4, 1.0f);
	};

	BENCHMARK("construct from initializer list dim=4") {
		return Vectorf{ 1.0f, 2.0f, 3.0f, 4.0f };
	};

	BENCHMARK("operator+ dim=4") {
		Vectorf a{ 1.0f, 2.0f, 3.0f, 4.0f };
		Vectorf b{ 4.0f, 3.0f, 2.0f, 1.0f };
		return a + b;
	};

	BENCHMARK("operator+= dim=4") {
		Vectorf a{ 1.0f, 2.0f, 3.0f, 4.0f };
		Vectorf b{ 4.0f, 3.0f, 2.0f, 1.0f };
		a += b;
		return a;
	};

	BENCHMARK("length dim=4") {
		Vectorf v{ 1.0f, 2.0f, 3.0f, 4.0f };
		return length(v);
	};

	BENCHMARK("normalise dim=4") {
		Vectorf v{ 1.0f, 2.0f, 3.0f, 4.0f };
		return normalise(v);
	};

	BENCHMARK("dot dim=4") {
		Vectorf a{ 1.0f, 2.0f, 3.0f, 4.0f };
		Vectorf b{ 4.0f, 3.0f, 2.0f, 1.0f };
		return dot(a, b);
	};
}

TEST_CASE("rotation benchmarks", "[!benchmark][rotation]") {

	BENCHMARK("plane_rotate dim=4") {
		Vectorf v{ 1.0f, 2.0f, 3.0f, 4.0f };
		return plane_rotate(v, 0, 1, 0.5f);
	};

	BENCHMARK("rotate dim=4") {
		Vectorf v{ 1.0f, 2.0f, 3.0f, 4.0f };
		Vectorf angles(6, 0.5f);
		return rotate(v, angles);
	};

	BENCHMARK("build_rotation_matrix dim=4") {
		Vectorf angles(6, 0.5f);
		return build_rotation_matrix(4, angles);
	};

	BENCHMARK("rotate dim=8") {
		Vectorf v(8, 1.0f);
		Vectorf angles(28, 0.5f);
		return rotate(v, angles);
	};

	BENCHMARK("build_rotation_matrix dim=8") {
		Vectorf angles(28, 0.5f);
		return build_rotation_matrix(8, angles);
	};
}
