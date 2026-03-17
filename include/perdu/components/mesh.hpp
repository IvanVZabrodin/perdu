#pragma once

#include "perdu/assets/asset_cache.hpp"
#include "perdu/assets/assets.hpp"
#include "perdu/core/maths.hpp"
#include "perdu/renderer/mesh.hpp"

#include <cstdint>
#include <vector>
namespace perdu {
	struct Mesh
	{
		PrimitiveType primitive_type = PrimitiveType::Triangles;

		uint32_t			  dim;
		std::vector<Vectorf>  vertices;
		std::vector<uint32_t> indices;

		MeshHandle handle{};

		void recompute();
		void recompute_via_cpumesh(CPUMesh& cpu) const;
	};

	inline Mesh make_cube(uint32_t		dim,
						  float			half_extent = 1.0f,
						  PrimitiveType mode		= PrimitiveType::Lines) {
		Mesh mesh;
		mesh.primitive_type = mode;
		mesh.dim			= dim;

		uint32_t vertex_count = 1u << dim;
		mesh.vertices.reserve(vertex_count);
		for (uint32_t mask = 0; mask < vertex_count; ++mask) {
			Vectorf v(dim, 0.0f);
			for (uint32_t i = 0; i < dim; ++i)
				v[i] = (mask >> i & 1) ? half_extent : -half_extent;
			mesh.vertices.push_back(v);
		}

		switch (mode) {
			case PrimitiveType::Points:
				// One index per vertex
				for (uint32_t i = 0; i < vertex_count; ++i)
					mesh.indices.push_back(i);
				break;

			case PrimitiveType::Lines:
				// Pairs of vertices differing in exactly one bit
				for (uint32_t a = 0; a < vertex_count; ++a) {
					for (uint32_t b = a + 1; b < vertex_count; ++b) {
						uint32_t diff = a ^ b;
						if (diff && (diff & (diff - 1)) == 0) {
							mesh.indices.push_back(a);
							mesh.indices.push_back(b);
						}
					}
				}
				break;

			case perdu::PrimitiveType::Triangles:
				// if (dim == 3) {
				// 	mesh.indices = { 1, 0, 3, 3, 0, 2, 6, 4, 5, 7, 6, 5,
				// 					 0, 2, 4, 2, 6, 4, 5, 1, 3, 7, 5, 3,
				// 					 5, 0, 1, 4, 0, 5, 3, 2, 6, 3, 6, 7 };
				// 	return mesh;
				// }
				for (uint32_t axis_a = 0; axis_a < dim; ++axis_a) {
					for (uint32_t axis_b = axis_a + 1; axis_b < dim; ++axis_b) {
						uint32_t fixed_count = dim - 2;
						uint32_t face_count	 = 1u << fixed_count;

						for (uint32_t fixed = 0; fixed < face_count; ++fixed) {
							uint32_t base_mask = 0;
							uint32_t fixed_bit = 0;
							for (uint32_t ax = 0; ax < dim; ++ax) {
								if (ax == axis_a || ax == axis_b) continue;
								if (fixed >> fixed_bit & 1)
									base_mask |= (1u << ax);
								++fixed_bit;
							}

							uint32_t v00 = base_mask;
							uint32_t v10 = base_mask | (1u << axis_a);
							uint32_t v01 = base_mask | (1u << axis_b);
							uint32_t v11
							  = base_mask | (1u << axis_a) | (1u << axis_b);

							// Count set bits in base_mask to determine face
							// side odd = flip winding
							int fixed_axis = 0;
							for (int ax = 0; ax < dim; ++ax)
								if (ax != axis_a && ax != axis_b)
									fixed_axis = ax;

							// determine which side (0 or 1)
							uint32_t fixed_val = (base_mask >> fixed_axis) & 1;
							bool	 flip	   = fixed_val == 1;

							if (!flip) {
								mesh.indices.push_back(v00);
								mesh.indices.push_back(v10);
								mesh.indices.push_back(v11);

								mesh.indices.push_back(v00);
								mesh.indices.push_back(v11);
								mesh.indices.push_back(v01);
							} else {
								mesh.indices.push_back(v00);
								mesh.indices.push_back(v11);
								mesh.indices.push_back(v10);

								mesh.indices.push_back(v00);
								mesh.indices.push_back(v01);
								mesh.indices.push_back(v11);
							}
						}
					}
				}
				break;
		}
		return mesh;
	}
} // namespace perdu
