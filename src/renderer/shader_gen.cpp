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

void main() {
    uint global_idx = gl_GlobalInvocationID.x;
    if (global_idx >= pc.total_vertices) return;

	uint entity_idx = 0;
    uint running    = 0;
    for (uint i = 0; i < pc.entity_count; ++i) {
        if (global_idx < running + entities[i].vertex_count) {
            entity_idx = i;
            break;
        }
        running += entities[i].vertex_count;
    }

	EntityInfo e   = entities[entity_idx];
    uint local_idx = global_idx - e.vertex_offset;

	float v[DIM];
    for (uint i = 0; i < DIM; ++i)
        v[i] = vertices[e.vertex_offset * DIM + local_idx * DIM + i];

	float rotated1[DIM];
    for (uint i = 0; i < DIM; ++i) {
        rotated1[i] = 0.0;
        for (uint j = 0; j < DIM; ++j)
            rotated1[i] += transforms[e.matrix_offset + i * DIM + j] * v[j];
    }

	for (uint i = 0; i < DIM; ++i)
        rotated1[i] += transforms[e.position_offset + i];
	
	float translated[DIM];
	for (uint i = 0; i < DIM; ++i)
		translated[i] = rotated1[i] - camtran[i];

	// Step 2: rotate by camera orientation
	float rotated[DIM];
	for (uint i = 0; i < DIM; ++i) {
		rotated[i] = 0.0;
		for (uint j = 0; j < DIM; ++j)
			rotated[i] += cammat[i * DIM + j] * translated[j];  // j*DIM+i instead of i*DIM+j
	}

	
	float current[DIM];
    for (uint i = 0; i < DIM; ++i) current[i] = rotated[i];
	
	bool visible = true;

    for (uint d = DIM; d > 2; --d) {
        float w     = e.camera_dist - current[d - 1];
		if (w < 0.0001) {
			visible = false;
			break;
		}
        float scale = e.camera_dist / w;
        for (uint i = 0; i < d - 1; ++i)
            current[i] *= scale;
    }


	float w     = e.camera_dist - rotated[2];
	if (!visible) {
		// Push vertex far off screen so it doesn't affect rendering
		projected[global_idx] = vec4(0.0, 0.0, -2.0, v[0]);
		return;
	}
	float depth = e.camera_dist / w;  // perspective depth
	depth       = clamp(depth * 0.5 + 0.5, 0.0, 1.0);  // map to [0,1]
	

	// current[0] = x, current[1] = y
	// rotated[2] = depth (z before final projection)
	// 1.0        = w
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
