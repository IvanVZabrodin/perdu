#include "perdu/assets/assets.hpp"
#include "perdu/components/mesh.hpp"
#include "perdu/renderer/gpu_context.hpp"
#include "renderer/directories.hpp"
#include "renderer/shader.hpp"

#include <filesystem>
#include <memory>
#include <string>

namespace perdu {
	std::string asset_path(std::string relative) {
		return (rootdir / relative).string();
	}

	ShaderAsset* load_shader(GPUContext* ctx, const CPUShader& cpu) {
		ShaderAsset* a = new ShaderAsset{
			cpu,
			std::make_shared<GPUShader>(load_shader_from_cpushader(ctx, cpu))
		};
		return a;
	}

	// MeshAsset* load_mesh(GPUContext& ctx, const Mesh& mesh) {
	// 	CPUMesh cpu{ mesh.dim, mesh.primitive_type };
	// 	mesh.recompute_via_cpumesh(cpu);
	// 	GPUMesh gpu{ ctx };
	// 	return new MeshAsset(cpu, gpu);
	// }
}
