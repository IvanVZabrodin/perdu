#include "perdu/core/maths.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace perdu;
using namespace Catch::Matchers;

static void check_all_zero(const Vectorf& v, float eps = 1e-5f) {
	for (size_t i = 0; i < v.dim(); ++i) {
		INFO("component " << i);
		CHECK_THAT(v[i], WithinAbs(0.0f, eps));
	}
}

static void check_equal(const Vectorf& a, const Vectorf& b, float eps = 1e-5f) {
	REQUIRE(a.dim() == b.dim());
	for (size_t i = 0; i < a.dim(); ++i) {
		INFO("component " << i);
		CHECK_THAT(a[i], WithinAbs(b[i], eps));
	}
}

static const float PI = 3.14159265358979323846f;

TEST_CASE("Vector", "[core][maths]") {

	SECTION("construction") {

		SECTION("zero initialised") {
			Vectorf v(4);
			REQUIRE(v.dim() == 4);
			check_all_zero(v);
		}

		SECTION("fill value") {
			Vectorf v(4, 2.0f);
			REQUIRE(v.dim() == 4);
			for (size_t i = 0; i < 4; ++i) CHECK(v[i] == 2.0f);
		}

		SECTION("initializer list") {
			Vectorf v{ 1.0f, 2.0f, 3.0f, 4.0f };
			REQUIRE(v.dim() == 4);
			CHECK(v[0] == 1.0f);
			CHECK(v[1] == 2.0f);
			CHECK(v[2] == 3.0f);
			CHECK(v[3] == 4.0f);
		}

		SECTION("dim 1") {
			Vectorf v(1, 5.0f);
			REQUIRE(v.dim() == 1);
			CHECK(v[0] == 5.0f);
		}
	}

	SECTION("operator[]") {

		SECTION("read") {
			Vectorf v{ 1.0f, 2.0f, 3.0f };
			CHECK(v[0] == 1.0f);
			CHECK(v[2] == 3.0f);
		}

		SECTION("write") {
			Vectorf v(3);
			v[1] = 7.0f;
			CHECK(v[1] == 7.0f);
		}

		SECTION("const read") {
			const Vectorf v{ 1.0f, 2.0f };
			CHECK(v[0] == 1.0f);
		}
	}

	SECTION("dim()") {
		CHECK(Vectorf(1).dim() == 1);
		CHECK(Vectorf(4).dim() == 4);
		CHECK(Vectorf(10).dim() == 10);
		CHECK(Vectorf{ 1.0f, 2.0f, 3.0f }.dim() == 3);
	}

	SECTION("data()") {

		SECTION("contiguous and matches operator[]") {
			Vectorf		 v{ 1.0f, 2.0f, 3.0f, 4.0f };
			const float* ptr = v.data();
			for (size_t i = 0; i < 4; ++i) CHECK(ptr[i] == v[i]);
		}

		SECTION("const overload") {
			const Vectorf v{ 1.0f, 2.0f };
			const float*  ptr = v.data();
			CHECK(ptr[0] == 1.0f);
		}
	}

	SECTION("iteration") {
		Vectorf v{ 1.0f, 2.0f, 3.0f };
		float	expected = 1.0f;
		for (float x : v) {
			CHECK(x == expected);
			expected += 1.0f;
		}
	}

	SECTION("arithmetic") {
		Vectorf a{ 1.0f, 2.0f, 3.0f };
		Vectorf b{ 4.0f, 5.0f, 6.0f };

		SECTION("addition") {
			Vectorf c = a + b;
			CHECK(c[0] == 5.0f);
			CHECK(c[1] == 7.0f);
			CHECK(c[2] == 9.0f);
		}

		SECTION("addition does not modify operands") {
			Vectorf c = a + b;
			CHECK(a[0] == 1.0f);
			CHECK(b[0] == 4.0f);
		}

		SECTION("subtraction") {
			Vectorf c = b - a;
			CHECK(c[0] == 3.0f);
			CHECK(c[1] == 3.0f);
			CHECK(c[2] == 3.0f);
		}

		SECTION("scalar multiply") {
			Vectorf c = a * 2.0f;
			CHECK(c[0] == 2.0f);
			CHECK(c[1] == 4.0f);
			CHECK(c[2] == 6.0f);
		}

		SECTION("scalar divide") {
			Vectorf c = a / 2.0f;
			CHECK(c[0] == 0.5f);
			CHECK(c[1] == 1.0f);
			CHECK(c[2] == 1.5f);
		}
	}

	SECTION("compound assignment") {
		SECTION("+=") {
			Vectorf a{ 1.0f, 2.0f, 3.0f };
			Vectorf b{ 4.0f, 5.0f, 6.0f };
			a += b;
			CHECK(a[0] == 5.0f);
			CHECK(a[1] == 7.0f);
			CHECK(a[2] == 9.0f);
		}

		SECTION("-=") {
			Vectorf a{ 4.0f, 5.0f, 6.0f };
			Vectorf b{ 1.0f, 2.0f, 3.0f };
			a -= b;
			CHECK(a[0] == 3.0f);
			CHECK(a[1] == 3.0f);
			CHECK(a[2] == 3.0f);
		}

		SECTION("*=") {
			Vectorf a{ 1.0f, 2.0f, 3.0f };
			a *= 3.0f;
			CHECK(a[0] == 3.0f);
			CHECK(a[1] == 6.0f);
			CHECK(a[2] == 9.0f);
		}

		SECTION("/=") {
			Vectorf a{ 2.0f, 4.0f, 6.0f };
			a /= 2.0f;
			CHECK(a[0] == 1.0f);
			CHECK(a[1] == 2.0f);
			CHECK(a[2] == 3.0f);
		}

		SECTION("+= returns self reference") {
			Vectorf	 a{ 1.0f, 2.0f };
			Vectorf	 b{ 3.0f, 4.0f };
			Vectorf& ref = (a += b);
			CHECK(&ref == &a);
		}
	}

	SECTION("equality") {
		Vectorf a{ 1.0f, 2.0f, 3.0f };
		Vectorf b{ 1.0f, 2.0f, 3.0f };
		Vectorf c{ 1.0f, 2.0f, 4.0f };

		CHECK(a == b);
		CHECK_FALSE(a == c);
	}

	SECTION("type conversion") {

		SECTION("float to int truncates") {
			Vectorf f{ 1.9f, 2.1f, 3.5f };
			auto	i = f.as<int>();
			REQUIRE(i.dim() == 3);
			CHECK(i[0] == 1);
			CHECK(i[1] == 2);
			CHECK(i[2] == 3);
		}

		SECTION("float to double preserves values") {
			Vectorf f{ 1.0f, 2.0f, 3.0f };
			auto	d = f.as<double>();
			REQUIRE(d.dim() == 3);
			CHECK_THAT(d[0], WithinRel(1.0, 1e-6));
			CHECK_THAT(d[1], WithinRel(2.0, 1e-6));
			CHECK_THAT(d[2], WithinRel(3.0, 1e-6));
		}

		SECTION("dim preserved across conversion") {
			Vectorf f(7, 1.0f);
			auto	i = f.as<int>();
			CHECK(i.dim() == 7);
		}
	}
}


TEST_CASE("math functions", "[core][maths]") {

	SECTION("dot") {

		SECTION("orthogonal vectors") {
			Vectorf a{ 1.0f, 0.0f, 0.0f, 0.0f };
			Vectorf b{ 0.0f, 1.0f, 0.0f, 0.0f };
			CHECK_THAT(dot(a, b), WithinAbs(0.0f, 1e-6f));
		}

		SECTION("parallel vectors") {
			Vectorf a{ 1.0f, 0.0f, 0.0f, 0.0f };
			CHECK_THAT(dot(a, a), WithinAbs(1.0f, 1e-6f));
		}

		SECTION("general case") {
			Vectorf a{ 1.0f, 2.0f, 3.0f };
			Vectorf b{ 4.0f, 5.0f, 6.0f };
			// 1*4 + 2*5 + 3*6 = 32
			CHECK_THAT(dot(a, b), WithinAbs(32.0f, 1e-5f));
		}

		SECTION("commutative") {
			Vectorf a{ 1.0f, 2.0f, 3.0f };
			Vectorf b{ 4.0f, 5.0f, 6.0f };
			CHECK_THAT(dot(a, b), WithinRel(dot(b, a), 1e-6f));
		}

		SECTION("zero vector") {
			Vectorf a{ 1.0f, 2.0f, 3.0f };
			Vectorf z(3, 0.0f);
			CHECK_THAT(dot(a, z), WithinAbs(0.0f, 1e-6f));
		}
	}

	SECTION("length") {

		SECTION("3-4-5 triangle") {
			Vectorf v{ 3.0f, 4.0f };
			CHECK_THAT(length(v), WithinRel(5.0f, 1e-5f));
		}

		SECTION("zero vector") {
			Vectorf v(4, 0.0f);
			CHECK_THAT(length(v), WithinAbs(0.0f, 1e-6f));
		}

		SECTION("unit vector") {
			Vectorf v{ 1.0f, 0.0f, 0.0f, 0.0f };
			CHECK_THAT(length(v), WithinRel(1.0f, 1e-6f));
		}

		SECTION("4D") {
			// length of {1,1,1,1} = sqrt(4) = 2
			Vectorf v(4, 1.0f);
			CHECK_THAT(length(v), WithinRel(2.0f, 1e-5f));
		}
	}

	SECTION("normalise") {

		SECTION("result has length 1") {
			Vectorf v{ 3.0f, 4.0f, 0.0f };
			CHECK_THAT(length(normalise(v)), WithinRel(1.0f, 1e-5f));
		}

		SECTION("already normalised is unchanged") {
			Vectorf v{ 1.0f, 0.0f, 0.0f, 0.0f };
			check_equal(normalise(v), v);
		}

		SECTION("direction is preserved") {
			Vectorf v{ 3.0f, 4.0f };
			Vectorf n = normalise(v);
			// ratio of components should be preserved
			CHECK_THAT(n[0] / n[1], WithinRel(3.0f / 4.0f, 1e-5f));
		}

		SECTION("does not modify original") {
			Vectorf v{ 3.0f, 4.0f };
			normalise(v);
			CHECK(v[0] == 3.0f);
			CHECK(v[1] == 4.0f);
		}
	}
}

TEST_CASE("plane_index", "[core][maths]") {

	SECTION("all planes for dim=4") {
		CHECK(plane_index(4, 0, 1) == 0);
		CHECK(plane_index(4, 0, 2) == 1);
		CHECK(plane_index(4, 0, 3) == 2);
		CHECK(plane_index(4, 1, 2) == 3);
		CHECK(plane_index(4, 1, 3) == 4);
		CHECK(plane_index(4, 2, 3) == 5);
	}

	SECTION("all planes for dim=3") {
		CHECK(plane_index(3, 0, 1) == 0);
		CHECK(plane_index(3, 0, 2) == 1);
		CHECK(plane_index(3, 1, 2) == 2);
	}

	SECTION("total plane count is correct for dim 2 to 6") {
		for (size_t dim = 2; dim <= 6; ++dim) {
			size_t expected = dim * (dim - 1) / 2;
			// highest index + 1 should equal total plane count
			size_t highest	= plane_index(dim, dim - 2, dim - 1);
			CHECK(highest + 1 == expected);
		}
	}

	SECTION("dim=2 has one plane") {
		CHECK(plane_index(2, 0, 1) == 0);
	}
}


TEST_CASE("rotation", "[core][maths]") {

	SECTION("plane_rotate") {

		SECTION("zero angle returns original") {
			Vectorf v{ 1.0f, 2.0f, 3.0f, 4.0f };
			Vectorf r = plane_rotate(v, 0, 1, 0.0f);
			check_equal(r, v);
		}

		SECTION("2pi returns original") {
			Vectorf v{ 1.0f, 2.0f, 3.0f, 4.0f };
			Vectorf r = plane_rotate(v, 0, 1, 2.0f * PI);
			check_equal(r, v);
		}

		SECTION("only modifies axes a and b") {
			Vectorf v{ 1.0f, 2.0f, 3.0f, 4.0f };
			Vectorf r = plane_rotate(v, 0, 1, 0.5f);
			// axes 2 and 3 should be unchanged
			CHECK(r[2] == v[2]);
			CHECK(r[3] == v[3]);
		}

		SECTION("pi/2 rotation on axes 0,1") {
			// {1,0,0,0} rotated pi/2 in (0,1) plane -> {0,1,0,0}
			Vectorf v{ 1.0f, 0.0f, 0.0f, 0.0f };
			Vectorf r = plane_rotate(v, 0, 1, PI / 2.0f);
			CHECK_THAT(r[0], WithinAbs(0.0f, 1e-5f));
			CHECK_THAT(r[1], WithinAbs(1.0f, 1e-5f));
			CHECK_THAT(r[2], WithinAbs(0.0f, 1e-5f));
			CHECK_THAT(r[3], WithinAbs(0.0f, 1e-5f));
		}

		SECTION("pi rotation reverses both axes") {
			Vectorf v{ 1.0f, 0.0f, 0.0f, 0.0f };
			Vectorf r = plane_rotate(v, 0, 1, PI);
			CHECK_THAT(r[0], WithinAbs(-1.0f, 1e-5f));
			CHECK_THAT(r[1], WithinAbs(0.0f, 1e-5f));
		}

		SECTION("preserves length") {
			Vectorf v{ 3.0f, 4.0f, 0.0f, 0.0f };
			float	original = length(v);
			Vectorf r		 = plane_rotate(v, 0, 1, 1.23f);
			CHECK_THAT(length(r), WithinRel(original, 1e-5f));
		}
	}

	SECTION("rotate") {

		SECTION("zero angles returns original") {
			Vectorf v{ 1.0f, 2.0f, 3.0f, 4.0f };
			Vectorf angles(6, 0.0f);
			check_equal(rotate(v, angles), v);
		}

		SECTION("preserves length") {
			Vectorf v{ 1.0f, 2.0f, 3.0f, 4.0f };
			float	original = length(v);

			Vectorf angles(6, 0.0f);
			angles[0] = 0.5f;
			angles[3] = 1.2f;
			angles[5] = 0.8f;

			CHECK_THAT(length(rotate(v, angles)), WithinRel(original, 1e-5f));
		}

		SECTION("2pi on all planes returns original") {
			Vectorf v{ 1.0f, 2.0f, 3.0f, 4.0f };
			Vectorf angles(6, 2.0f * PI);
			check_equal(rotate(v, angles), v);
		}

		SECTION("matches sequential plane_rotate calls") {
			Vectorf v{ 1.0f, 0.0f, 0.0f, 0.0f };
			Vectorf angles(6, 0.0f);
			angles[0] = 0.5f; // plane (0,1)
			angles[2] = 0.3f; // plane (0,3)

			Vectorf via_rotate = rotate(v, angles);

			Vectorf manual = v;
			manual		   = plane_rotate(manual, 0, 1, 0.5f);
			manual		   = plane_rotate(manual, 0, 3, 0.3f);

			check_equal(via_rotate, manual);
		}
	}

	SECTION("build_rotation_matrix") {

		SECTION("zero angles produces identity") {
			size_t			   dim = 4;
			Vectorf			   angles(6, 0.0f);
			std::vector<float> mat = build_rotation_matrix(dim, angles);

			for (size_t i = 0; i < dim; ++i) {
				for (size_t j = 0; j < dim; ++j) {
					float expected = (i == j) ? 1.0f : 0.0f;
					INFO("mat[" << i << "][" << j << "]");
					CHECK_THAT(mat[i * dim + j], WithinAbs(expected, 1e-5f));
				}
			}
		}

		SECTION("R times R transpose is identity") {
			size_t	dim = 4;
			Vectorf angles(6, 0.0f);
			angles[0] = 0.5f;
			angles[1] = 1.2f;
			angles[4] = 0.8f;

			std::vector<float> mat = build_rotation_matrix(dim, angles);

			// Compute R * R^T
			for (size_t i = 0; i < dim; ++i) {
				for (size_t j = 0; j < dim; ++j) {
					float sum = 0.0f;
					for (size_t k = 0; k < dim; ++k)
						sum += mat[i * dim + k] * mat[j * dim + k];
					float expected = (i == j) ? 1.0f : 0.0f;
					INFO("product[" << i << "][" << j << "]");
					CHECK_THAT(sum, WithinAbs(expected, 1e-4f));
				}
			}
		}

		SECTION("applying matrix to vector matches rotate()") {
			size_t	dim = 4;
			Vectorf v{ 1.0f, 2.0f, 3.0f, 4.0f };
			Vectorf angles(dim * (dim - 1) / 2, 0.0f);
			angles[0] = 0.5f;
			angles[3] = 1.2f;

			std::vector<float> mat = build_rotation_matrix(dim, angles);
			Vectorf			   via_mat(dim, 0.0f);
			for (size_t i = 0; i < dim; ++i)
				for (size_t k = 0; k < dim; ++k)
					via_mat[i] += mat[i * dim + k] * v[k]; // row-major mat * v

			Vectorf via_rotate = rotate(v, angles);
			check_equal(via_mat, via_rotate, 1e-4f);
		}

		SECTION("works for dim=3") {
			size_t	dim = 3;
			Vectorf angles(3, 0.0f);
			angles[0]			   = 1.0f;
			std::vector<float> mat = build_rotation_matrix(dim, angles);
			REQUIRE(mat.size() == 9);

			// Check orthogonality
			for (size_t i = 0; i < dim; ++i) {
				for (size_t j = 0; j < dim; ++j) {
					float sum = 0.0f;
					for (size_t k = 0; k < dim; ++k)
						sum += mat[i * dim + k] * mat[j * dim + k];
					float expected = (i == j) ? 1.0f : 0.0f;
					CHECK_THAT(sum, WithinAbs(expected, 1e-4f));
				}
			}
		}
	}
}
