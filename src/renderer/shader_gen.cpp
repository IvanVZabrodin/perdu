#include "perdu/core/assert.hpp"
#include "perdu/core/log.hpp"
#include "renderer/shader.hpp"

#include <cstdint>
#include <shaderc/shaderc.h>
#include <shaderc/shaderc.hpp>
#include <shaderc/status.h>
#include <string>
#include <vector>

namespace perdu {
	static const char* PROJECTION_GLSL = R"(
#version 450
layout(local_size_x = 64) in;

struct EntityInfo {
    uint  vertex_offset;
    uint  vertex_count;
    uint  matrix_offset;
    uint  position_offset;
    float camera_dist;
    uint  dim;
    uint  _pad[2];
};

layout(std430, set = 0, binding = 0) readonly buffer EntityBuffer {
    EntityInfo entities[];
};
layout(std430, set = 0, binding = 1) readonly buffer VertexBuffer {
    float vertices[];
};
layout(std430, set = 0, binding = 2) readonly buffer TransformBuffer {
    float transforms[];
};
layout(std430, set = 0, binding = 3) readonly buffer CameraBuffer {
    float cammat[DIM * DIM];
    float camtran[DIM];
};
layout(std430, set = 1, binding = 0) writeonly buffer OutputBuffer {
    vec4 projected[];
};
layout(std140, set = 2, binding = 0) uniform PC {
    uint total_vertices;
    uint entity_count;
} pc;

// Cache camtran and cammat in shared memory — all 64 threads in workgroup use them
shared float s_cammat[DIM * DIM];
shared float s_camtran[DIM];

void main() {
    uint global_idx = gl_GlobalInvocationID.x;
    uint local_id   = gl_LocalInvocationID.x;

    // Cooperatively load camera data into shared memory
    // DIM*DIM + DIM floats total, spread across threads
    uint cam_floats = DIM * DIM + DIM;
    for (uint i = local_id; i < cam_floats; i += 64) {
        if (i < DIM * DIM)
            s_cammat[i] = cammat[i];
        else
            s_camtran[i - DIM * DIM] = camtran[i - DIM * DIM];
    }
    barrier();
    memoryBarrierShared();

    if (global_idx >= pc.total_vertices) return;

    // Binary search for entity — O(log n) vs O(n)
    // Requires vertex_offset to be sorted ascending (it will be, allocation is sequential)
    uint lo = 0, hi = pc.entity_count - 1, entity_idx = 0;
    while (lo <= hi) {
        uint mid = (lo + hi) >> 1;
        uint end = entities[mid].vertex_offset + entities[mid].vertex_count;
        if (global_idx < entities[mid].vertex_offset) {
            hi = mid - 1;
        } else if (global_idx >= end) {
            lo = mid + 1;
        } else {
            entity_idx = mid;
            break;
        }
    }

    EntityInfo e   = entities[entity_idx];
    uint local_idx = global_idx - e.vertex_offset;

    // Load vertex
    float v[DIM];
    uint vbase = e.vertex_offset * DIM + local_idx * DIM;
    for (uint i = 0; i < DIM; ++i)
        v[i] = vertices[vbase + i];

    // Transform: rotate then translate in one pass to avoid a second temp array
    float rotated1[DIM];
    uint moff = e.matrix_offset;
    for (uint i = 0; i < DIM; ++i) {
        float acc = 0.0;
        for (uint j = 0; j < DIM; ++j)
            acc += transforms[moff + i * DIM + j] * v[j];
        rotated1[i] = acc + transforms[e.position_offset + i];
    }

    // Camera: translate then rotate, using shared memory
    float rotated[DIM];
    for (uint i = 0; i < DIM; ++i) {
        float acc = 0.0;
        for (uint j = 0; j < DIM; ++j)
            acc += s_cammat[j * DIM + i] * (rotated1[j] - s_camtran[j]);
        rotated[i] = acc;
    }

    // Iterative perspective projection
    float current[DIM];
    for (uint i = 0; i < DIM; ++i) current[i] = rotated[i];

    for (uint d = DIM; d > 2; --d) {
        float w = e.camera_dist - current[d - 1];
        if (w < 0.0001) {
            projected[global_idx] = vec4(0.0, 0.0, -2.0, v[0]);
            return;
        }
        float scale = e.camera_dist / w;
        for (uint i = 0; i < d - 1; ++i)
            current[i] *= scale;
    }

    float w     = e.camera_dist - rotated[2];
    float depth = clamp((e.camera_dist / w) * 0.5 + 0.5, 0.0, 1.0);

    projected[global_idx] = vec4(current[0], current[1], depth, v[0]);
}
	)";

	static std::vector<uint32_t> compile_projection(uint32_t dim) {
		shaderc::Compiler		comp{};
		shaderc::CompileOptions opts{};

		opts.AddMacroDefinition("DIM", std::to_string(dim));
		opts.SetOptimizationLevel(shaderc_optimization_level_performance);
		opts.SetTargetEnvironment(shaderc_target_env_vulkan,
								  shaderc_env_version_vulkan_1_0);
		auto res = comp.CompileGlslToSpv(
		  PROJECTION_GLSL, shaderc_compute_shader, "project.comp", opts);

		PERDU_ASSERT(res.GetCompilationStatus()
					   == shaderc_compilation_status_success,
					 "failed to compile projection shader with error "
					   + res.GetErrorMessage());
		return { res.begin(), res.end() };
	}

	ComputeAsset* generate_projection_shader(GPUContext& ctx, uint32_t dim) {
		auto code = compile_projection(dim);

		CPUCompute cpu{
			.spirv		  = code,
			.buffcount	  = { 4, 1 },
			.textcount	  = { 0, 0 },
			.uniformcount = 1,
			.samplers	  = 0,
			.threadx	  = 64,
			.thready	  = 1,
			.threadz	  = 1
		};

		return new ComputeAsset{ cpu, GPUCompute(ctx, cpu) };
	}

	ComputeAsset* ComputeCache::get(uint32_t dim) {
		auto it = computes.find(dim);

		if (it != computes.end()) return it->second;

		ComputeAsset* c = generate_projection_shader(ctx, dim);
		computes[dim]	= c;
		return computes[dim];
	}

	ComputeCache::ComputeCache(GPUContext& __ctx) : ctx(__ctx) {};
	ComputeCache::~ComputeCache() {
		for (auto& [dim, c] : computes) { delete c; }
	}
}
