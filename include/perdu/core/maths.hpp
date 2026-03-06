#pragma once

#include "perdu/core/assert.hpp"

#include <cmath>
#include <concepts>
#include <initializer_list>
#include <ostream>
#include <sstream>
#include <vector>

namespace perdu {
	inline constexpr size_t rotation_planes(size_t dim) {
		return dim * (dim - 1) / 2;
	}

	template <typename T>
	class Vector {
	  private:
		size_t		   _dim = 0;
		std::vector<T> _data;

	  public:
		Vector() : _dim(0) {}
		explicit Vector(size_t dim, T val = T{}) : _dim(dim), _data(dim, val) {}

		Vector(std::initializer_list<T> values) :
			_dim(values.size()), _data(values.begin(), values.end()) {}

		template <typename NT>
		Vector<NT> as() const {
			Vector<NT> res(_dim);
			std::copy(_data.begin(), _data.end(), res.begin());
			return res;
		}

		auto begin() { return _data.begin(); }
		auto end() { return _data.end(); }
		auto begin() const { return _data.cbegin(); }
		auto end() const { return _data.cend(); }

		T*		 data() { return _data.data(); }
		const T* data() const { return _data.data(); }

		size_t dim() const { return _dim; }

		T&		 operator[](size_t i) { return _data[i]; }
		const T& operator[](size_t i) const { return _data[i]; }

		Vector& operator+=(const Vector& __other) {
			PERDU_ASSERT(_dim == __other.dim(), "dim mismatch");
			for (auto a = begin(), b = __other.begin(); a != end(); a++, b++) {
				*a += *b;
			}
			return *this;
		}

		Vector& operator-=(const Vector& __other) {
			PERDU_ASSERT(_dim == __other.dim(), "dim mismatch");
			for (auto a = begin(), b = __other.begin(); a != end(); a++, b++) {
				*a -= *b;
			}
			return *this;
		}

		Vector& operator*=(T __scalar) {
			for (auto a = begin(); a != end(); ++a) *a *= __scalar;
			return *this;
		}

		Vector& operator/=(T __scalar) {
			for (auto a = begin(); a != end(); ++a) *a /= __scalar;
			return *this;
		}

		bool operator==(const Vector& __other) const {
			if (_dim != __other.dim()) return false;
			for (auto a = begin(), b = __other.begin(); a != end(); ++a, ++b) {
				if (*a != *b) return false;
			}
			return true;
		}
	};

	template <typename T>
	Vector<T> operator+(Vector<T> lhs, const Vector<T>& rhs) {
		lhs += rhs;
		return lhs;
	}

	template <typename T>
	Vector<T> operator-(Vector<T> lhs, const Vector<T>& rhs) {
		lhs -= rhs;
		return lhs;
	}

	template <typename T>
	Vector<T> operator*(Vector<T> lhs, T rhs) {
		lhs *= rhs;
		return lhs;
	}

	template <typename T>
	Vector<T> operator/(Vector<T> lhs, T rhs) {
		lhs /= rhs;
		return lhs;
	}

	template <typename T>
	std::ostream& operator<<(std::ostream& os, const Vector<T>& vec) {
		os << "(";
		for (size_t i = 0; i < vec.dim(); ++i) {
			os << vec[i];
			if (i != vec.dim() - 1) os << ", ";
		}
		return os << ")";
	}

	template <typename T>
	std::string to_string(const Vector<T>& v) {
		std::ostringstream ss;
		ss << v;
		return ss.str();
	}

	using Vectorf = Vector<float>;
	using Vectord = Vector<double>;
	using Vectori = Vector<int>;

	template <typename T>
	T dot(const Vector<T>& a, const Vector<T>& b) {
		PERDU_ASSERT(a.dim() == b.dim(), "dim mismatch");
		T s{};
		for (size_t i = 0; i < a.dim(); ++i) {
			s += a[i] * b[i];
		}
		return s;
	}

	template <typename T>
	inline T length(const Vector<T>& vec) {
		return std::sqrt(dot(vec, vec));
	}


	template <typename T>
	Vector<T> normalise(Vector<T> vec) {
		return vec / length(vec);
	}

	inline size_t plane_index(size_t dim, size_t a, size_t b) {
		PERDU_ASSERT(a < b, "a must be less than b");
		PERDU_ASSERT(b < dim, "b must be less than dim");
		return a * dim - (a * (a + 1)) / 2 + b - a - 1;
	}

	template <std::floating_point T, std::floating_point RT = T>
	Vector<T> plane_rotate(Vector<T> vec, size_t dima, size_t dimb, RT theta) {
		PERDU_ASSERT(dima < vec.dim() && dimb < vec.dim(), "out of range dim");
		RT c = std::cos(theta), s = std::sin(theta);
		T  a = vec[dima], b = vec[dimb];
		vec[dima] = a * c - b * s;
		vec[dimb] = a * s + b * c;

		return vec;
	}

	template <std::floating_point T, std::floating_point RT = T>
	Vector<T> rotate(Vector<T> vec, const Vector<RT>& angles) {
		PERDU_ASSERT(angles.dim() == rotation_planes(vec.dim()),
					 "invalid dim pair");
		for (size_t i = 0, ind = 0; i < vec.dim() - 1; ++i) {
			for (size_t j = i + 1; j < vec.dim(); ++j, ++ind) {
				RT c = std::cos(angles[ind]);
				RT s = std::sin(angles[ind]);
				T  a = vec[i], b = vec[j];
				vec[i] = a * c - b * s;
				vec[j] = a * s + b * c;
			}
		}
		return vec;
	}

	template <std::floating_point T = float, std::floating_point RT = T>
	std::vector<T> build_rotation_matrix(size_t dim, const Vector<RT>& angles) {
		PERDU_ASSERT(angles.dim() == rotation_planes(dim), "invalid dim pair");
		std::vector<T> mat(dim * dim, 0.0f);
		for (size_t i = 0; i < dim; ++i) mat[i * dim + i] = 1.0f;

		for (size_t a = 0; a < dim - 1; ++a) {
			for (size_t b = a + 1; b < dim; ++b) {
				RT theta = angles[plane_index(dim, a, b)];
				T  c = std::cos(theta), s = std::sin(theta);

				for (size_t col = 0; col < dim; ++col) {
					T ra			   = mat[a * dim + col];
					T rb			   = mat[b * dim + col];
					mat[a * dim + col] = ra * c - rb * s;
					mat[b * dim + col] = ra * s + rb * c;
				}
			}
		}

		return mat;
	}
}
