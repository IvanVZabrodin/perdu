#include "perdu/components/mesh.hpp"

#include "perdu/renderer/mesh.hpp"

#include <vector>

namespace perdu {
	void Mesh::recompute() {
		recompute_via_cpumesh(handle->cpu);
	}

	void Mesh::recompute_via_cpumesh(CPUMesh& cpu) const {
		std::vector<float> flat;
		flat.reserve(vertices.size() * dim);

		auto i = flat.begin();

		for (auto& v : vertices) flat.insert(i, v.begin(), v.end());
		cpu.vertices	   = std::move(flat);
		cpu.primitive_type = primitive_type;
		cpu.dim			   = dim;
		cpu.indices		   = std::move(indices);
		cpu.dirty		   = true;
	}
}
