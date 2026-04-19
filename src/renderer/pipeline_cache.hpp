#pragma once

#include "perdu/assets/asset_cache.hpp"
#include "perdu/renderer/gpu_context.hpp"
#include "perdu/renderer/mesh.hpp"
#include "perdu/renderer/pipeline.hpp"
#include "renderer/pipeline.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <unordered_map>
#include <utility>

struct TupleHash
{
	template <class... Ts>
	std::size_t operator()(const std::tuple<Ts...>& t) const {
		std::size_t seed = 0;

		std::apply(
		  [&](const auto&... xs) {
			  ((seed ^= std::hash<std::decay_t<decltype(xs)>>{}(xs)
					  + 0x9e3779b9
					  + (seed << 6)
					  + (seed >> 2)),
			   ...);
		  },
		  t);

		return seed;
	}
};

namespace perdu {
	class PipelineCache {
	  public:
		PipelineCache(GPUContext& ctx);

		Pipeline& get(ShaderHandle		  vert,
					  ShaderHandle		  frag,
					  PrimitiveType		  primitive_type,
					  const PipelineInfo& info = {});
		Pipeline  create(const PipelineInfo& info);

		bool exists(ShaderHandle  vert,
					ShaderHandle  frag,
					PrimitiveType primitive_type) const;
		void clear() { _pipelines.clear(); }

	  private:
		using Key = std::tuple<uint32_t, uint32_t, PrimitiveType>;

		GPUContext& _ctx;
		std::unordered_map<Key, std::unique_ptr<Pipeline>, TupleHash>
		  _pipelines;
	};
}
