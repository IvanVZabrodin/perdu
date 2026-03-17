#pragma once

#include "perdu/assets/assets.hpp"
#include "perdu/assets/resource_cache.hpp"
#include "perdu/renderer/gpu_context.hpp"
#include "perdu/renderer/shader.hpp"

namespace perdu {
	using ShaderHandle = ResourceCache<ShaderAsset>::Handle;
	using MeshHandle   = ResourceCache<MeshAsset>::Handle;

	struct AssetCache
	{
		ResourceCache<ShaderAsset> shaders;
		ResourceCache<MeshAsset>   meshes;

		// ShaderHandle upload();
		MeshHandle upload_mesh(const Mesh& mesh);
	};
}
