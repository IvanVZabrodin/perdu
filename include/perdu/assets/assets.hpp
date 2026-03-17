#pragma once

#include "perdu/renderer/gpu_context.hpp"
#include "perdu/renderer/mesh.hpp"
#include "perdu/renderer/shader.hpp"

#include <optional>

namespace perdu {
	struct Mesh;

	template <typename CPU, typename GPU = void>
	struct Asset
	{
		CPU cpu;
		GPU gpu;
	};

	std::string asset_path(std::string relative);

	using ShaderAsset = Asset<CPUShader, GPUShader>;
	ShaderAsset* load_shader(GPUContext& ctx, const CPUShader& cpu);

	using MeshAsset = Asset<CPUMesh, GPUMesh>;
	MeshAsset* load_mesh(GPUContext& ctx, const Mesh& mesh);
}
