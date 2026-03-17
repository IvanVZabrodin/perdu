#include "perdu/renderer/renderer.hpp"

#include "perdu/assets/assets.hpp"
#include "perdu/components/mesh.hpp"
#include "perdu/components/transform.hpp"
#include "perdu/core/assert.hpp"
#include "perdu/core/log.hpp"
#include "perdu/core/maths.hpp"
#include "perdu/engine/scene.hpp"
#include "perdu/renderer/gpu_context.hpp"
#include "perdu/renderer/mesh.hpp"
#include "perdu/renderer/pipeline.hpp"
#include "renderer/pipeline.hpp"
#include "renderer/pipeline_cache.hpp"
#include "renderer/renderer.hpp"
#include "renderer/shader.hpp"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <memory>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <string>
#include <unordered_map>
#include <vector>

void read_output_buffer(SDL_GPUDevice*				 device,
						SDL_GPUBuffer*				 output_buffer,
						uint32_t					 vertex_count,
						std::vector<perdu::Vectorf>& out) {
	uint32_t size = vertex_count * sizeof(float) * 4;

	// Create download transfer buffer
	SDL_GPUTransferBufferCreateInfo tb_info{};
	tb_info.usage			  = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
	tb_info.size			  = size;
	SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device, &tb_info);

	// Copy output buffer → transfer buffer
	SDL_GPUCommandBuffer* cmd  = SDL_AcquireGPUCommandBuffer(device);
	SDL_GPUCopyPass*	  copy = SDL_BeginGPUCopyPass(cmd);

	SDL_GPUBufferRegion			  src{ output_buffer, 0, size };
	SDL_GPUTransferBufferLocation dst{ tb, 0 };
	SDL_DownloadFromGPUBuffer(copy, &src, &dst);

	SDL_EndGPUCopyPass(copy);

	// Fence to wait for GPU to finish
	SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
	SDL_WaitForGPUFences(device, true, &fence, 1);
	SDL_ReleaseGPUFence(device, fence);

	// Map and read
	float* mapped = (float*) SDL_MapGPUTransferBuffer(device, tb, false);
	out.resize(vertex_count);
	for (uint32_t i = 0; i < vertex_count; ++i) {
		out[i] = {
			mapped[i * 4 + 0], // x
			mapped[i * 4 + 1], // y
			mapped[i * 4 + 2], // depth
			mapped[i * 4 + 3]  // w
		};
	}
	SDL_UnmapGPUTransferBuffer(device, tb);
	SDL_ReleaseGPUTransferBuffer(device, tb);
}

void debug_read_transfer_buffer(SDL_GPUDevice* device,
								SDL_GPUBuffer* gpu_buffer,
								uint32_t	   size) {
	// Create download transfer buffer
	SDL_GPUTransferBufferCreateInfo tb_info{};
	tb_info.usage			  = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
	tb_info.size			  = size;
	SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device, &tb_info);

	// Copy GPU buffer → transfer buffer
	SDL_GPUCommandBuffer* cmd  = SDL_AcquireGPUCommandBuffer(device);
	SDL_GPUCopyPass*	  copy = SDL_BeginGPUCopyPass(cmd);

	SDL_GPUBufferRegion			  src{ gpu_buffer, 0, size };
	SDL_GPUTransferBufferLocation dst{ tb, 0 };
	SDL_DownloadFromGPUBuffer(copy, &src, &dst);

	SDL_EndGPUCopyPass(copy);

	// Wait for GPU to finish
	SDL_GPUFence* fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
	SDL_WaitForGPUFences(device, true, &fence, 1);
	SDL_ReleaseGPUFence(device, fence);

	// Read
	float*	 mapped = (float*) SDL_MapGPUTransferBuffer(device, tb, false);
	uint32_t count	= size / sizeof(float);
	for (uint32_t i = 0; i < count; ++i)
		PERDU_LOG_DEBUG(
		  "float[" + std::to_string(i) + "] = " + std::to_string(mapped[i]));

	SDL_UnmapGPUTransferBuffer(device, tb);
	SDL_ReleaseGPUTransferBuffer(device, tb);
}


namespace perdu {
	Renderer::Renderer(GPUContext& ctx, Scene& scene) :
		_ctx(ctx), _scene(scene) {
		SDL_SetHint(SDL_HINT_RENDER_VULKAN_DEBUG, "1");
		_pipelines = std::make_unique<PipelineCache>(_ctx);
		_computes  = std::make_unique<ComputeCache>(_ctx);

		_scene.registry.on_construct<Mesh>()
		  .connect<&Renderer::on_mesh_construct>(*this);
		_scene.registry.on_destroy<Mesh>().connect<&Renderer::on_mesh_destruct>(
		  *this);

		_scene.registry.on_construct<Camera>()
		  .connect<&Renderer::on_mesh_construct>(*this);
		_scene.registry.on_destroy<Camera>()
		  .connect<&Renderer::on_mesh_destruct>(*this);

		PERDU_LOG_INFO("renderer initialised");
	}

	Renderer::~Renderer() {
		for (auto& [dim, bufs] : _dim_buffers) {
			if (bufs.entity_buffer)
				SDL_ReleaseGPUBuffer(_ctx.device, bufs.entity_buffer);
			if (bufs.vertex_buffer)
				SDL_ReleaseGPUBuffer(_ctx.device, bufs.vertex_buffer);
			if (bufs.transform_buffer)
				SDL_ReleaseGPUBuffer(_ctx.device, bufs.transform_buffer);
			if (bufs.output_buffer)
				SDL_ReleaseGPUBuffer(_ctx.device, bufs.output_buffer);
		}

		if (_transfer) SDL_ReleaseGPUTransferBuffer(_ctx.device, _transfer);

		PERDU_LOG_INFO("renderer destroyed");
	}

	void Renderer::on_resize(uint32_t width, uint32_t height) {
		PERDU_LOG_DEBUG("reacting to resize");
	}

	void Renderer::on_mesh_construct(entt::registry& reg, entt::entity e) {
		const Mesh& m = reg.get<Mesh>(e);
		reg.emplace<RenderState>(e, m.dim);
		reg.emplace<TransformCache>(e);
	}

	void Renderer::on_mesh_destruct(entt::registry& reg, entt::entity e) {
		reg.remove<RenderState>(e);
		reg.remove<TransformCache>(e);
	}

	void Renderer::on_cam_construct(entt::registry& reg, entt::entity e) {
		reg.emplace<TransformCache>(e);
	}

	void Renderer::on_cam_destruct(entt::registry& reg, entt::entity e) {
		reg.remove<TransformCache>(e);
	}


	void Renderer::begin_frame() {
		// auto s = std::chrono::system_clock::now();
		SDL_WaitForGPUIdle(_ctx.device);
		// auto e = std::chrono::system_clock::now();
		// auto d = e - s;
		// PERDU_LOG_DEBUG(
		//   std::to_string(std::chrono::duration<float>(d).count()));
		_cmd = SDL_AcquireGPUCommandBuffer(_ctx.device);

		target->load_texture(_cmd);
		SDL_GPUTexture* ctext = target->colour;
		// if (!SDL_AcquireGPUSwapchainTexture(
		// 	  _cmd, _ctx.window, &swapchain, nullptr, nullptr)
		// 	|| swapchain == nullptr)
		// {
		// 	SDL_SubmitGPUCommandBuffer(_cmd);
		// 	_cmd  = nullptr;
		// 	_pass = nullptr;
		// 	return;
		// }

		SDL_GPUColorTargetInfo color{
			.texture	 = ctext,
			.clear_color = { 0.1f, 0.1f, 0.1f, 1.0f },
			.load_op	 = SDL_GPU_LOADOP_CLEAR,
			.store_op	 = SDL_GPU_STOREOP_STORE,
		};

		SDL_GPUDepthStencilTargetInfo depth{
			.texture	 = target->depth,
			.clear_depth = 0.0001f,
			.load_op	 = SDL_GPU_LOADOP_CLEAR,
			.store_op	 = SDL_GPU_STOREOP_DONT_CARE,
		};

		_pass = SDL_BeginGPURenderPass(_cmd, &color, 1, &depth);
	}

	void Renderer::prerender() {
		collect_transforms();
		collect_meshes();
		compute_dispatch();
	}

	void Renderer::render() {
		_scene.registry.view<Mesh, RenderState>().each([&](Mesh&		m,
														   RenderState& state) {
			auto& pipeline
			  = _pipelines->get(vert,
								frag,
								m.primitive_type,
								{ .colour_format = target->colour_format });
			pipeline.bind(_pass);

			auto& bufs = get_dim_buffers(m.dim, 0, 0, 0);

			SDL_GPUBufferBinding vb{ bufs.output_buffer,
									 (uint32_t) (state.voff * sizeof(float)) };

			SDL_BindGPUVertexBuffers(_pass, 0, &vb, 1);

			GPUMesh&			 gpu = m.handle->gpu;
			SDL_GPUBufferBinding ib{ gpu.inds, 0 };
			SDL_BindGPUIndexBuffer(_pass, &ib, SDL_GPU_INDEXELEMENTSIZE_32BIT);
			SDL_DrawGPUIndexedPrimitives(
			  _pass, gpu.isize / sizeof(uint32_t), 1, 0, 0, 0);
		});
	}

	void Renderer::end_frame() {
		if (!_pass || !_cmd) return;
		SDL_EndGPURenderPass(_pass);
		SDL_SubmitGPUCommandBuffer(_cmd);
		// SDL_WaitForGPUIdle(_ctx.device);
		_cmd  = nullptr;
		_pass = nullptr;
	}

	SDL_GPUTransferBuffer* Renderer::get_transfer_buffer(uint32_t size) {
		if (size <= _tsize) return _transfer;
		PERDU_LOG_DEBUG("resizing transfer buffer to " + std::to_string(size));
		if (_transfer) SDL_ReleaseGPUTransferBuffer(_ctx.device, _transfer);

		SDL_GPUTransferBufferCreateInfo info{
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, .size = size
		};

		_transfer = SDL_CreateGPUTransferBuffer(_ctx.device, &info);
		_tsize	  = size;
		PERDU_ASSERT(_transfer, "failed to create transfer buffer");
		return _transfer;
	}

	Renderer::DimBuffers& Renderer::get_dim_buffers(uint32_t dim,
													uint32_t entsize,
													uint32_t vsize,
													uint32_t tsize) {
		auto& bufs = _dim_buffers[dim];

		uint32_t esize	  = entsize * sizeof(EntityInfo);
		uint32_t vertsize = vsize * dim * sizeof(float);
		uint32_t transize = tsize * sizeof(float);
		uint32_t outsize  = vsize * sizeof(float) * 4;

		ensure_dim_buf(bufs.entity_buffer,
					   bufs.entity_capacity,
					   esize,
					   SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ);
		ensure_dim_buf(bufs.vertex_buffer,
					   bufs.vertex_capacity,
					   vertsize,
					   SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ);
		ensure_dim_buf(bufs.transform_buffer,
					   bufs.transform_capacity,
					   transize,
					   SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ);
		ensure_dim_buf(bufs.output_buffer,
					   bufs.output_capacity,
					   outsize,
					   SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE
						 | SDL_GPU_BUFFERUSAGE_VERTEX);

		return bufs;
	}

	void Renderer::ensure_dim_buf(SDL_GPUBuffer*& buf,
								  uint32_t&		  size,
								  uint32_t		  required,
								  uint32_t		  usage) {
		if (required <= size) return;
		if (buf != nullptr) SDL_ReleaseGPUBuffer(_ctx.device, buf);

		SDL_GPUBufferCreateInfo info{ .usage = usage, .size = required };

		buf	 = SDL_CreateGPUBuffer(_ctx.device, &info);
		size = required;
	}

	void Renderer::collect_meshes() {

		std::unordered_map<uint32_t, uint32_t> dim_offsets;

		_scene.registry.view<Mesh, RenderState>().each(
		  [&](Mesh& m, RenderState& rs) {
			  uint32_t& offset = dim_offsets[m.dim];
			  rs.voff		   = offset;
			  rs.vcount		   = (uint32_t) m.vertices.size();
			  rs.dim		   = m.dim;
			  offset		  += rs.vcount;
		  });

		struct DirtyEntry
		{
			MeshAsset*	 asset;
			RenderState* render_state;
			uint32_t	 vcount;
		};

		std::unordered_map<uint32_t, std::vector<DirtyEntry>> dimdirty;

		// uint32_t totib_size = 0;
		// uint32_t totvb_size = 0;

		_scene.registry.view<Mesh, RenderState>().each([&](Mesh&		mesh,
														   RenderState& state) {
			if (!mesh.handle->cpu.dirty) return;
			dimdirty[mesh.dim].push_back(
			  { &mesh.handle.get(), &state, (uint32_t) mesh.vertices.size() });
			// dirty.push_back(
			//   { &mesh.handle.get(), &state, (uint32_t) mesh.vertices.size()
			//   });
			// totvb_size +=
			// static_cast<uint32_t>(mesh.handle->cpu.vertices.size()
			// 									* sizeof(float));
			// totib_size +=
			// static_cast<uint32_t>(mesh.handle->cpu.indices.size()
			// 									* sizeof(uint32_t));
		});

		// if (dirty.empty()) return;
		//
		_precmd = SDL_AcquireGPUCommandBuffer(_ctx.device);

		for (auto& [dim, meshes] : dimdirty) {
			uint32_t tibs = 0;
			uint32_t tvbs = 0;

			for (auto& [asset, rs, vc] : meshes) {
				tvbs += (uint32_t) (asset->cpu.vertices.size() * sizeof(float));
				tibs
				  += (uint32_t) (asset->cpu.indices.size() * sizeof(uint32_t));
			}
			SDL_GPUTransferBuffer* tb = get_transfer_buffer(tibs + tvbs);
			uint8_t*			   mapped
			  = (uint8_t*) SDL_MapGPUTransferBuffer(_ctx.device, tb, true);

			SDL_GPUCopyPass* copy  = SDL_BeginGPUCopyPass(_precmd);
			uint32_t		 vboff = 0;
			uint32_t		 iboff = tvbs;

			for (auto& [asset, rs, vc] : meshes) {
				CPUMesh& cpu = asset->cpu;

				uint32_t vb_size
				  = static_cast<uint32_t>(cpu.vertices.size() * sizeof(float));
				uint32_t ib_size = static_cast<uint32_t>(cpu.indices.size()
														 * sizeof(uint32_t));

				memcpy(mapped + vboff, cpu.vertices.data(), vb_size);
				memcpy(mapped + iboff, cpu.indices.data(), ib_size);

				asset->gpu.ensure_indbuf(ib_size);

				auto& dimbuffs = get_dim_buffers(dim, 0, vc, 0);

				SDL_GPUTransferBufferLocation vbsrc{ tb, vboff };
				SDL_GPUBufferRegion			  vbdst{
					  dimbuffs.vertex_buffer,
					  rs->voff * dim * (uint32_t) sizeof(float),
					  vb_size
				};
				SDL_UploadToGPUBuffer(copy, &vbsrc, &vbdst, false);

				SDL_GPUTransferBufferLocation ibsrc{ tb, iboff };
				SDL_GPUBufferRegion ibdst{ asset->gpu.inds, 0, ib_size };
				SDL_UploadToGPUBuffer(copy, &ibsrc, &ibdst, false);

				vboff += vb_size;
				iboff += ib_size;

				asset->cpu.dirty = false;
			}

			SDL_UnmapGPUTransferBuffer(_ctx.device, tb);
			SDL_EndGPUCopyPass(copy);
			// SDL_SubmitGPUCommandBuffer(cmd);

			// auto& bufs = get_dim_buffers(dim, 0, 0, 0);
			// debug_read_transfer_buffer(_ctx.device, bufs.vertex_buffer,
			// tvbs);
		}
	}

	void Renderer::collect_transforms() {
		_scene.registry.view<Transform, TransformCache, Mesh>().each(
		  [&](Transform& t, TransformCache& cache, Mesh& mesh) {
			  if (t.position == cache.last_pos
				  && t.rotation == cache.last_rot
				  && !cache._dirty)
				  return;
			  auto mat		 = build_rotation_matrix(t.dim(), t.rotation);
			  cache.rotmat	 = std::move(mat);
			  cache.last_rot = t.rotation;
			  cache.last_pos = t.position;
			  cache._dirty	 = false;
		  });
	}

	void Renderer::compute_dispatch() {
		struct DimGroup
		{
			std::vector<EntityInfo> entity_infos;
			std::vector<float>		transform_data;
			std::vector<float>		vertex_data;
			uint32_t				vtot = 0;
		};

		std::unordered_map<uint32_t, DimGroup> groups;

		_scene.registry.view<Mesh, RenderState, TransformCache>().each(
		  [&](Mesh& mesh, RenderState& state, TransformCache& cache) {
			  DimGroup& dg = groups[mesh.dim];

			  EntityInfo info{
				  .voff	  = dg.vtot,
				  .vcount = state.vcount,
				  .moff	  = (uint32_t) dg.transform_data.size(),
				  .poff
				  = (uint32_t) (dg.transform_data.size() + cache.rotmat.size()),
				  .camera_dist = 2.0f,
				  .dim		   = mesh.dim,
			  };

			  dg.entity_infos.push_back(info);

			  dg.transform_data.insert(dg.transform_data.end(),
									   cache.rotmat.begin(),
									   cache.rotmat.end());
			  dg.transform_data.insert(dg.transform_data.end(),
									   cache.last_pos.begin(),
									   cache.last_pos.end());
			  dg.vtot += state.vcount;
		  });

		if (groups.empty()) return;

		// SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(_ctx.device);

		for (auto& [dim, group] : groups) {
			if (group.entity_infos.empty()) continue;

			auto  c = _computes->get(dim);
			auto& bufs
			  = get_dim_buffers(dim,
								(uint32_t) group.entity_infos.size(),
								group.vtot,
								(uint32_t) group.transform_data.size());

			uint32_t entity_size
			  = (uint32_t) (group.entity_infos.size() * sizeof(EntityInfo));
			uint32_t transform_size
			  = (uint32_t) (group.transform_data.size() * sizeof(float));
			uint32_t totsize = entity_size + transform_size;

			SDL_GPUTransferBuffer* tb = get_transfer_buffer(totsize);
			uint8_t*			   mapped
			  = (uint8_t*) SDL_MapGPUTransferBuffer(_ctx.device, tb, true);

			memcpy(mapped, group.entity_infos.data(), entity_size);
			memcpy(mapped + entity_size,
				   group.transform_data.data(),
				   transform_size);

			SDL_UnmapGPUTransferBuffer(_ctx.device, tb);

			SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(_precmd);

			SDL_GPUTransferBufferLocation entity_src{ tb, 0 };
			SDL_GPUBufferRegion			  entity_dst{ bufs.entity_buffer,
											  0,
											  entity_size };
			SDL_UploadToGPUBuffer(copy, &entity_src, &entity_dst, false);

			SDL_GPUTransferBufferLocation transform_src{ tb, entity_size };
			SDL_GPUBufferRegion			  transform_dst{ bufs.transform_buffer,
												 0,
												 transform_size };
			SDL_UploadToGPUBuffer(copy, &transform_src, &transform_dst, false);

			SDL_EndGPUCopyPass(copy);

			SDL_GPUStorageBufferReadWriteBinding output_binding{};
			output_binding.buffer = bufs.output_buffer;

			SDL_GPUComputePass* pass = SDL_BeginGPUComputePass(
			  _precmd, nullptr, 0, &output_binding, 1);

			c->gpu.bind(pass);

			SDL_GPUBuffer* storage_bufs[] = {
				bufs.entity_buffer,
				bufs.vertex_buffer,
				bufs.transform_buffer,
			};
			SDL_BindGPUComputeStorageBuffers(pass, 0, storage_bufs, 3);

			struct PC
			{
				uint32_t total_vertices;
				uint32_t entity_count;
			} pc{ group.vtot, (uint32_t) group.entity_infos.size() };
			SDL_PushGPUComputeUniformData(_precmd, 0, &pc, sizeof(pc));

			uint32_t groups_count = (group.vtot + 63) / 64;
			SDL_DispatchGPUCompute(pass, groups_count, 1, 1);

			SDL_EndGPUComputePass(pass);
		}

		SDL_SubmitGPUCommandBuffer(_precmd);

		// for (auto& [dim, group] : groups) {
		// 	auto& bufs = get_dim_buffers(dim, 0, 0, 0);
		//
		// 	std::vector<Vectorf> proj;
		// 	read_output_buffer(
		// 	  _ctx.device, bufs.output_buffer, group.vtot, proj);
		//
		// 	for (size_t i = 0; i < proj.size(); ++i) {
		// 		PERDU_LOG_DEBUG("dim "
		// 						+ std::to_string(dim)
		// 						+ " vertex "
		// 						+ std::to_string(i)
		// 						+ ": "
		// 						+ perdu::to_string(proj[i]));
		// 	}
		// }
	}
}
