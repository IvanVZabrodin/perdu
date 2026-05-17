#include "perdu/core/log.hpp"
#include "perdu/core/maths.hpp"

#include <cstdint>
#define TINYOBJLOADER_IMPLEMENTATION

#include "perdu/components/mesh.hpp"
#include "perdu/renderer/mesh.hpp"
#include "tiny_obj_loader.h"

#include <vector>

namespace perdu {
	void Mesh::recompute() {
		recompute_via_cpumesh(handle->cpu);
	}

	void Mesh::recompute_via_cpumesh(CPUMesh& cpu) const {
		std::vector<float> flat;
		flat.reserve(vertices.size() * dim);

		for (auto& v : vertices) flat.insert(flat.end(), v.begin(), v.end());
		cpu.vertices	   = std::move(flat);
		cpu.primitive_type = primitive_type;
		cpu.dim			   = dim;
		cpu.indices		   = std::move(indices);
		cpu.dirty		   = true;
	}

	Mesh load_mesh_from_obj(std::string file) {
		tinyobj::ObjReaderConfig reader_config;
		reader_config.triangulate	  = true;
		reader_config.mtl_search_path = "./";

		tinyobj::ObjReader reader;

		if (!reader.ParseFromFile(file, reader_config)) {
			if (!reader.Error().empty()) {
				PERDU_LOG_WARN("TinyObjReader: " + reader.Error());
			}
			return {};
		}
		if (!reader.Warning().empty()) {
			PERDU_LOG_WARN("TinyObjReader: " + reader.Warning());
		}

		auto& attrib = reader.GetAttrib();
		auto& shapes = reader.GetShapes();

		std::vector<uint32_t> inds;
		std::vector<Vectorf>  verts;

		uint32_t i = 0;
		for (auto& shape : shapes) {
			for (auto& index : shape.mesh.indices) {
				verts.push_back(
				  { attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2] });
				inds.push_back(i++);
			}
		}

		PERDU_LOG_DEBUG("vertex count: " + std::to_string(verts.size()));

		// for (size_t i = 0; i < attrib.vertices.size(); i += 3) {
		// 	verts.push_back({ attrib.vertices[i],
		// 					  attrib.vertices[i + 1],
		// 					  attrib.vertices[i + 2] });
		// }

		return { PrimitiveType::Triangles, 3, verts, inds };
	}
}
