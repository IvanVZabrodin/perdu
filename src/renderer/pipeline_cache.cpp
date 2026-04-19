#include "pipeline_cache.hpp"

#include "perdu/assets/asset_cache.hpp"
#include "perdu/core/log.hpp"
#include "perdu/renderer/gpu_context.hpp"
#include "perdu/renderer/mesh.hpp"
#include "perdu/renderer/pipeline.hpp"
#include "renderer/pipeline.hpp"

#include <memory>
#include <tuple>
#include <utility>


namespace perdu {
	PipelineCache::PipelineCache(GPUContext& ctx) : _ctx(ctx) {}

	Pipeline& PipelineCache::get(ShaderHandle		 vert,
								 ShaderHandle		 frag,
								 PrimitiveType		 primitive_type,
								 const PipelineInfo& info) {
		auto k	= std::make_tuple(vert.get_id(), frag.get_id(), primitive_type);
		auto it = _pipelines.find(k);
		if (it != _pipelines.end()) return *it->second;

		PipelineInfo inf   = info;
		inf.vert		   = vert;
		inf.frag		   = frag;
		inf.primitive_type = primitive_type;
		Pipeline p		   = create(inf);
		auto	 ptr	   = std::make_unique<Pipeline>(std::move(p));

		auto& c = _pipelines.emplace(k, std::move(ptr)).first->second;

		return *c;
	}

	Pipeline PipelineCache::create(const PipelineInfo& info) {
		PipelineDesc desc{ .vert		   = info.vert,
						   .frag		   = info.frag,
						   .attributes	   = info.vert->cpu.attributes,
						   .vertex_stride  = info.vert->cpu.vertex_strides,
						   .colour_format  = info.colour_format,
						   .primitive_type = info.primitive_type,
						   .depth_test	   = info.depth_test,
						   .depth_write	   = info.depth_write };
		return Pipeline(_ctx, desc);
	}

	bool PipelineCache::exists(ShaderHandle	 vert,
							   ShaderHandle	 frag,
							   PrimitiveType primitive_type) const {
		auto k = std::make_tuple(vert.get_id(), frag.get_id(), primitive_type);
		return _pipelines.contains(k);
	}
}
