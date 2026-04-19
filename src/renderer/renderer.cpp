#include "perdu/renderer/renderer.hpp"

#include "perdu/app/input.hpp"
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

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <string>
// #include <sys/types.h>
#include <unordered_map>
#include <vector>

static uint32_t align_size(uint32_t size, uint32_t chunk) {
	return (size + chunk - 1) / chunk * chunk;
}

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

	// Copy output buffer -> transfer buffer
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
	if (tb == nullptr) {
		PERDU_LOG_ERROR(SDL_GetError());
		return;
	}

	// Copy GPU buffer -> transfer buffer
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
		  .connect<&Renderer::on_cam_construct>(*this);
		_scene.registry.on_destroy<Camera>()
		  .connect<&Renderer::on_cam_destruct>(*this);

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
			if (bufs.cam_buffer)
				SDL_ReleaseGPUBuffer(_ctx.device, bufs.cam_buffer);
			if (bufs.output_buffer)
				SDL_ReleaseGPUBuffer(_ctx.device, bufs.output_buffer);
		}

		if (_lastfence) SDL_ReleaseGPUFence(_ctx.device, _lastfence);
		if (_transfer) SDL_ReleaseGPUTransferBuffer(_ctx.device, _transfer);
		if (_inds) SDL_ReleaseGPUBuffer(_ctx.device, _inds);

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

	RenderOffsets Renderer::allocate_for_dim(uint32_t dim,
											 uint32_t size,
											 uint32_t indsize) {
		RenderOffsets coff = _dimtooff[dim];
		coff.indicies	   = _indoff;
		_indoff			  += indsize;
		_dimtooff[dim]	   = { .vert	  = coff.vert + size,
							   .transform = coff.transform + dim * (dim + 1),
							   .entity	  = coff.entity + 1 };
		return coff;
	}


	void Renderer::begin_frame() {
		// auto s = std::chrono::system_clock::now();
		// auto e = std::chrono::system_clock::now();
		// auto d = e - s;
		// PERDU_LOG_DEBUG(
		//   std::to_string(std::chrono::duration<float>(d).count()));
		_cmd = SDL_AcquireGPUCommandBuffer(_ctx.device);

		GPUTexture col = view->target->load_texture(_cmd);
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
			.texture	 = col.texture,
			.clear_color = { 0.1f, 0.1f, 0.1f, 1.0f },
			.load_op	 = SDL_GPU_LOADOP_CLEAR,
			.store_op	 = SDL_GPU_STOREOP_STORE,
		};

		SDL_GPUDepthStencilTargetInfo depth{
			.texture	 = view->target->depth.texture,
			.clear_depth = 0.0001f,
			.load_op	 = SDL_GPU_LOADOP_CLEAR,
			.store_op	 = SDL_GPU_STOREOP_DONT_CARE,
		};

		_pass = SDL_BeginGPURenderPass(_cmd, &color, 1, &depth);
	}

	/*
	 *	Prerender system
	 *	One copy pass
	 *	- multiple copies? maybe transform buf is always copied as whole -
	 * because it commonly changes? Only update the ones that are needed Cycling
	 * produces unknown values - so only use if it is a good idea.
	 * Transfer buffer is the max size
	 * */

	void Renderer::prerender() {
		// collect_meshes();
		// collect_transforms();
		// compute_dispatch();
		if (_prev_resize && _lastfence) {
			SDL_WaitForGPUFences(_ctx.device, true, &_lastfence, 1);
			SDL_ReleaseGPUFence(_ctx.device, _lastfence);
		}
		_prev_resize = false;
		_lastfence	 = nullptr;

		struct DirtyMesh
		{
			MeshAsset*	 mesh;
			RenderState* state;
			bool		 requires_entity = false;
		};

		struct DirtyTransform
		{
			TransformCache* cache;
			RenderState*	state;
		};

		struct DirtyDim
		{
			std::vector<DirtyMesh>		meshes;
			std::vector<DirtyTransform> transforms;
		};

		struct PC
		{
			uint32_t total_vertices;
			uint32_t entity_count;
		};

		std::unordered_map<uint32_t, DirtyDim> dirty;
		std::unordered_map<uint32_t, PC>	   uniforms;
		std::set<uint32_t>					   dims;

		_scene.registry
		  .view<Mesh,
				RenderState,
				Transform,
				TransformCache>() // Only renderable objects, not other objects
								  // with transformcache
		  .each([&](Mesh&			m,
					RenderState&	state,
					Transform&		transform,
					TransformCache& tcache) {
			  auto& cpu						  = m.handle->cpu;
			  uniforms[m.dim].entity_count	 += 1;
			  uniforms[m.dim].total_vertices += m.vertices.size();
			  dims.insert(m.dim);
			  if (!state.allocated) { // Allocate stable indices for
									  // unallocated meshes
				  state.offsets = allocate_for_dim(
					state.dim,
					cpu.vertices.size(),
					cpu.indices.size()); // TODO: Choose which size is best
				  state.allocated = true;
				  state.vcount	  = m.vertices.size();
				  dirty[state.dim].meshes.push_back(
					{ &m.handle.get(), &state, true });
				  // PERDU_LOG_DEBUG("allocated at: { vert = "
				  // 		  + std::to_string(state.offsets.vert)
				  // 		  + ", transform = "
				  // 		  + std::to_string(state.offsets.transform)
				  // 		  + ", entity = "
				  // 		  + std::to_string(state.offsets.entity)
				  // 		  + " }");
				  cpu.dirty = false;
			  } else if (cpu.dirty) {
				  dirty[state.dim].meshes.push_back(
					{ &m.handle.get(), &state });
				  cpu.dirty = false;
			  }

			  if (!tcache.compare(transform)
				  || tcache._dirty) { // Dirty transform
				  tcache.last_pos = transform.position;
				  tcache.last_rot = transform.rotation;
				  tcache.rotmat
					= build_rotation_matrix(state.dim, tcache.last_rot);
				  tcache._dirty = false;
				  dirty[state.dim].transforms.push_back({ &tcache, &state });
			  }
		  });

		Transform&		camt = _scene.registry.get<Transform>(view->camera);
		TransformCache& camc
		  = _scene.registry.get<TransformCache>(view->camera);
		bool camdirty = !camc.compare(camt) || camc._dirty;
		if (camdirty) {
			camc.last_pos = camt.position;
			camc.last_rot = camt.rotation;

			camc._dirty = false;
		}

		if (dirty.empty() && !camdirty) return;
		SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(_ctx.device);

		_prev_resize |= ensure_dim_buf(_inds,
									   _isize,
									   _indoff * sizeof(uint32_t),
									   SDL_GPU_BUFFERUSAGE_INDEX,
									   true,
									   cmd);
		PERDU_LOG_DEBUG(std::to_string(_indoff * sizeof(uint32_t)));

		for (auto dim : dims) {
			auto& dirt = dirty[dim];
			if (!camdirty && dirt.meshes.empty() && dirt.transforms.empty())
				continue;

			auto [dimbufs, resize]
			  = get_dim_buffers(dim,
								_dimtooff[dim].entity * sizeof(EntityInfo),
								_dimtooff[dim].vert * sizeof(float),
								_dimtooff[dim].transform * sizeof(float),
								cmd);
			_prev_resize |= resize;

			if (dirt.transforms.size()
				> 2) { /* then do full transform write later? */
			}

			uint32_t totsize = (dirt.transforms.size() + (camdirty ? 1 : 0))
							 * dim
							 * (dim + 1)
							 * sizeof(float);

			for (auto& mesh : dirt.meshes) {
				totsize += mesh.mesh->cpu.vertices.size() * sizeof(float)
						 + mesh.mesh->cpu.indices.size() * sizeof(uint32_t)
						 + (mesh.requires_entity ? sizeof(EntityInfo) : 0);
			}

			struct CopyMap
			{
				SDL_GPUTransferBufferLocation src;
				SDL_GPUBufferRegion			  dst;
				// std::string					  debugname = "unkown";
			};

			std::vector<CopyMap> copies{};
			uint32_t			 coff = 0;

			SDL_GPUTransferBuffer* tb = get_transfer_buffer(totsize);
			uint8_t*			   mapped
			  = (uint8_t*) SDL_MapGPUTransferBuffer(_ctx.device, tb, false);

			if (camdirty) {
				auto v		= camc.last_pos.extend(dim);
				auto rotmat = build_rotation_matrix(
				  dim, camc.last_rot.extend(rotation_planes(dim)), true);
				memcpy(mapped + coff, rotmat.data(), dim * dim * sizeof(float));
				copies.push_back({
					.src = { tb, coff },
					.dst = { dimbufs.cam_buffer,
							 0, dim * (dim + 1) * (uint32_t) sizeof(float) }
				  });
				coff += dim * dim * sizeof(float);
				memcpy(mapped + coff, v.data(), dim * sizeof(float));
				coff += dim * sizeof(float);
			}

			for (auto& mesh : dirt.meshes) {
				CPUMesh& cpu = mesh.mesh->cpu;

				if (mesh.requires_entity) {
					EntityInfo info{ .voff	 = mesh.state->offsets.vert,
									 .vcount = mesh.state->vcount,
									 .moff	 = mesh.state->offsets.transform,
									 .poff
									 = mesh.state->offsets.transform
									 + mesh.state->dim * (mesh.state->dim),
									 .camera_dist = 2.0f,
									 .dim		  = mesh.state->dim };
					memcpy(mapped + coff, &info, sizeof(EntityInfo));
					copies.push_back({
						.src = { tb, coff },
						.dst = { dimbufs.entity_buffer,
								 mesh.state->offsets.entity
								   * (uint32_t) sizeof(EntityInfo),
								 sizeof(EntityInfo) },
						// .debugname = "entity"
					});
					coff += sizeof(EntityInfo);
				}

				memcpy(mapped + coff,
					   cpu.vertices.data(),
					   cpu.vertices.size() * sizeof(float));
				copies.push_back({
					.src = { tb, coff },
					.dst
					= { dimbufs.vertex_buffer,
							mesh.state->offsets.vert * (uint32_t) sizeof(float),
							(uint32_t) cpu.vertices.size()
						  * (uint32_t) sizeof(float),
						  },
					// .debugname = "vert"
				   });
				coff += cpu.vertices.size() * sizeof(float);

				// gpu.ensure_indbuf(cpu.indices.size() * sizeof(uint32_t));

				std::vector<uint32_t> offsetinds = cpu.indices;
				for (auto& i : offsetinds) {
					i += mesh.state->offsets.vert / dim;
					PERDU_LOG_DEBUG("inds: " + std::to_string(i));
				}

				memcpy(mapped + coff,
					   offsetinds.data(),
					   cpu.indices.size() * sizeof(uint32_t));

				copies.push_back({
					.src = { tb, coff },
					.dst = { _inds,
							 mesh.state->offsets.indicies
							   * (uint32_t) sizeof(uint32_t),
							 (uint32_t) cpu.indices.size()
							   * (uint32_t) sizeof(uint32_t) },
					// .debugname = "inds"
				});
				coff += cpu.indices.size() * sizeof(uint32_t);
			}

			for (auto& tran : dirt.transforms) {
				memcpy(mapped + coff,
					   tran.cache->rotmat.data(),
					   dim * dim * sizeof(float));
				// tran.state->transferoffsets.transform = coff;
				copies.push_back({
					.src = { tb, coff },
					.dst = { dimbufs.transform_buffer,
							 tran.state->offsets.transform
							   * (uint32_t) sizeof(float),
							 dim * (dim + 1) * (uint32_t) sizeof(float) },
					// .debugname = "trans"
				});
				coff += dim * dim * sizeof(float);
				memcpy(mapped + coff,
					   tran.cache->last_pos.data(),
					   dim * sizeof(float));
				coff += dim * sizeof(float);
			}


			SDL_UnmapGPUTransferBuffer(_ctx.device, tb);

			PERDU_LOG_DEBUG("cop " + std::to_string(copies.size()));
			if (!copies.empty()) {
				SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd);
				for (auto& [src, dst] : copies) {
					// PERDU_LOG_DEBUG("copy for "
					// 				+ debug
					// 				+ ": src off "
					// 				+ std::to_string(src.offset)
					// 				+ " dst off "
					// 				+ std::to_string(dst.offset)
					// 				+ " with size "
					// 				+ std::to_string(dst.size));
					SDL_UploadToGPUBuffer(copy, &src, &dst, false);
				}

				SDL_EndGPUCopyPass(copy);
			}

			// Dispatch here
			SDL_GPUStorageBufferReadWriteBinding output_binding{};
			output_binding.buffer = dimbufs.output_buffer;

			SDL_GPUComputePass* pass
			  = SDL_BeginGPUComputePass(cmd, nullptr, 0, &output_binding, 1);

			auto c = _computes->get(dim);
			c->gpu.bind(pass);

			SDL_GPUBuffer* storage_bufs[] = { dimbufs.entity_buffer,
											  dimbufs.vertex_buffer,
											  dimbufs.transform_buffer,
											  dimbufs.cam_buffer };
			SDL_BindGPUComputeStorageBuffers(pass, 0, storage_bufs, 4);

			SDL_PushGPUComputeUniformData(cmd, 0, &uniforms[dim], sizeof(PC));

			uint32_t groups_count = (uniforms[dim].total_vertices + 63) / 64;
			// PERDU_LOG_DEBUG("gcount: " + std::to_string(groups_count));
			SDL_DispatchGPUCompute(pass, groups_count, 1, 1);

			SDL_EndGPUComputePass(pass);


#ifndef NDEBUG
#endif
		}
		if (_prev_resize)
			_lastfence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
		else SDL_SubmitGPUCommandBuffer(cmd);

		// 	auto [dimbufs, _] = get_dim_buffers(3, 0, 0, 0);
		//
		// 	std::vector<Vectorf> proj;
		// 	read_output_buffer(
		// 	  _ctx.device, dimbufs.output_buffer, uniforms[3].total_vertices,
		// proj);
		//
		// 	for (size_t i = 0; i < proj.size(); ++i) {
		// 		PERDU_LOG_DEBUG("dim "
		// 						+ std::to_string(3)
		// 						+ " vertex "
		// 						+ std::to_string(i)
		// 						+ ": "
		// 						+ perdu::to_string(proj[i]));
		// 	}
	}

	void Renderer::render() {
		// SDL_WaitForGPUIdle(_ctx.device);
		SDL_GPUBufferBinding ib{ _inds, 0 };
		SDL_BindGPUIndexBuffer(_pass, &ib, SDL_GPU_INDEXELEMENTSIZE_32BIT);

		using BatchKey
		  = std::tuple<uint32_t, uint32_t, PrimitiveType, uint32_t>;


		struct DrawBatch
		{
			BatchKey key;
			uint32_t index_offset;
			uint32_t index_count;
		};

		std::unordered_map<BatchKey, uint32_t, TupleHash> batch_counts;
		_scene.registry.view<Mesh, RenderState>().each(
		  [&](Mesh& m, RenderState& state) {
			  batch_counts[{
				  vert.get_id(), frag.get_id(), m.primitive_type, m.dim }]
				+= m.handle->cpu.indices.size();
			  // PERDU_LOG_DEBUG(std::to_string(state.offsets.indicies));
		  });

		uint32_t index_offset = 0;
		for (auto& [key, index_count] : batch_counts) {
			auto [tvert, tfrag, primitive_type, dim] = key;
			auto& pipeline							 = _pipelines->get(
			  _scene.assets.shaders.get(tvert),
			  _scene.assets.shaders.get(tfrag),
			  primitive_type,
			  { .colour_format = view->target->colour.format });
			pipeline.bind(_pass);

			auto [bufs, _] = get_dim_buffers(dim, 0, 0, 0);
			SDL_GPUBufferBinding vb{ bufs.output_buffer, 0 };
			SDL_BindGPUVertexBuffers(_pass, 0, &vb, 1);
			PERDU_LOG_DEBUG(std::to_string(index_count));
			SDL_DrawGPUIndexedPrimitives(
			  _pass, index_count * 3, 1, index_offset, 0, 0);
			index_offset += index_count;
		}
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

	std::pair<Renderer::DimBuffers&, bool>
	  Renderer::get_dim_buffers(uint32_t			  dim,
								uint32_t			  entsize,
								uint32_t			  vsize,
								uint32_t			  tsize,
								SDL_GPUCommandBuffer* cmd) {
		auto& bufs = _dim_buffers[dim];

		uint32_t esize	  = entsize * sizeof(EntityInfo);
		uint32_t vertsize = vsize * dim * sizeof(float);
		uint32_t transize = tsize * sizeof(float);
		uint32_t outsize  = vsize * sizeof(float) * 4;
		uint32_t camsize  = dim * (dim + 1) * sizeof(float);
		bool	 needed	  = false;

		needed |= ensure_dim_buf(bufs.entity_buffer,
								 bufs.entity_capacity,
								 esize,
								 SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ,
								 true,
								 cmd);
		if (needed) PERDU_LOG_DEBUG("ent");
		needed |= ensure_dim_buf(bufs.vertex_buffer,
								 bufs.vertex_capacity,
								 vertsize,
								 SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ,
								 true,
								 cmd);
		if (needed) PERDU_LOG_DEBUG("vert");
		needed |= ensure_dim_buf(bufs.transform_buffer,
								 bufs.transform_capacity,
								 transize,
								 SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ,
								 true,
								 cmd);
		if (needed) PERDU_LOG_DEBUG("trans");
		needed |= ensure_dim_buf(bufs.output_buffer,
								 bufs.output_capacity,
								 outsize,
								 SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE
								   | SDL_GPU_BUFFERUSAGE_VERTEX,
								 false);
		uint32_t tesize = camsize;
		needed |= ensure_dim_buf(bufs.cam_buffer,
								 tesize,
								 camsize,
								 SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ,
								 false);
		return { bufs, needed };
	}

	bool Renderer::ensure_dim_buf(SDL_GPUBuffer*&		buf,
								  uint32_t&				size,
								  uint32_t				required,
								  uint32_t				usage,
								  bool					copy,
								  SDL_GPUCommandBuffer* cmd) {
		if (required <= size && buf != nullptr) return false;
		PERDU_LOG_DEBUG("prev required: " + std::to_string(required));
		required = align_size(required, chunk_size);
		PERDU_LOG_DEBUG("resize size: "
						+ std::to_string(required)
						+ ". actual size: "
						+ std::to_string(size));

		SDL_GPUBufferCreateInfo info{ .usage = usage, .size = required };

		SDL_GPUBuffer* nbuf = SDL_CreateGPUBuffer(_ctx.device, &info);
		if (copy && buf != nullptr) {
			PERDU_LOG_DEBUG("resizing and copying buf");
			SDL_GPUCopyPass*	  pass = SDL_BeginGPUCopyPass(cmd);
			SDL_GPUBufferLocation src{ .buffer = buf, .offset = 0 };
			SDL_GPUBufferLocation dst{ .buffer = nbuf, .offset = 0 };

			SDL_CopyGPUBufferToBuffer(pass, &src, &dst, size, false);
			SDL_EndGPUCopyPass(pass);
		}
		if (buf != nullptr) SDL_ReleaseGPUBuffer(_ctx.device, buf);
		buf	 = nbuf;
		size = required;
		return true;
	}

	// void Renderer::collect_meshes() {
	//
	// 	_precmd = SDL_AcquireGPUCommandBuffer(_ctx.device);
	// 	std::unordered_map<uint32_t, uint32_t> dim_offsets;
	//
	// 	auto v = _scene.registry.view<Mesh>();
	// 	for (auto it = v.rbegin(); it != v.rend(); ++it) {
	// 		auto&	  m		 = _scene.registry.get<Mesh>(*it);
	// 		auto&	  rs	 = _scene.registry.get<RenderState>(*it);
	// 		// PERDU_LOG_DEBUG("name " + m.handle->cpu.debugname);
	// 		uint32_t& offset = dim_offsets[m.dim];
	// 		rs.voff			 = offset;
	// 		rs.vcount		 = (uint32_t) m.vertices.size();
	// 		rs.dim			 = m.dim;
	// 		offset			+= rs.vcount;
	// 	};
	//
	// 	struct DirtyEntry
	// 	{
	// 		MeshAsset*	 asset;
	// 		RenderState* render_state;
	// 		uint32_t	 vcount;
	// 	};
	//
	// 	std::unordered_map<uint32_t, std::vector<DirtyEntry>> dimdirty;
	//
	// 	// uint32_t totib_size = 0;
	// 	// uint32_t totvb_size = 0;
	//
	// 	std::unordered_map<uint32_t, bool> founddirty;
	//
	// 	_scene.registry.view<Mesh, RenderState>().each([&](Mesh&		mesh,
	// 													   RenderState& state) {
	// 		if (!mesh.handle->cpu.dirty) return; // founddirty[mesh.dim] = true;
	// 		dimdirty[mesh.dim].push_back(
	// 		  { &mesh.handle.get(), &state, (uint32_t) mesh.vertices.size() });
	// 		// dirty.push_back(
	// 		//   { &mesh.handle.get(), &state, (uint32_t) mesh.vertices.size()
	// 		//   });
	// 		// totvb_size +=
	// 		// static_cast<uint32_t>(mesh.handle->cpu.vertices.size()
	// 		// 									* sizeof(float));
	// 		// totib_size +=
	// 		// static_cast<uint32_t>(mesh.handle->cpu.indices.size()
	// 		// 									* sizeof(uint32_t));
	// 	});
	//
	// 	// if (dimdirty.empty())
	// 	// 	return; // BAD BAD BAD BAD BAD THE GOOD ISNT WORKING FOR NOW NP BUT
	// 	// LIKE MAYBE FIX IDK
	// 	//
	//
	// 	/*
	// 	 * Current train of thought
	// 	 * Meshes won't change their vert count often (if at all). If they do,
	// 	 * either just resend all or move to back and keep a free tracker (will
	// 	 * kinda fragment but the buffer only grows so doesn't really matter)
	// 	 * So keep vertcount cached in renderstate, and if changed do that (and
	// 	 * if new just add to back) If they just change, lesser of two evils
	// 	 * type of thing: either recompute and send in one copy, but that means
	// 	 * that if it is the first and last of the meshes it will resend
	// 	 * everything - so probably not or send one at a time in one copy pass -
	// 	 * for commonly changing ones either use a separate buffer per or for
	// 	 * all dynamic meshes
	// 	 *
	// 	 * dynamic comes later
	// 	 * */
	// 	SDL_InsertGPUDebugLabel(_precmd, "collecting");
	//
	// 	for (auto& [dim, meshes] : dimdirty) {
	// 		// if (meshes.empty()) continue;
	// 		uint32_t tibs	 = 0;
	// 		uint32_t tvbs	 = 0;
	// 		uint32_t maxsize = 0;
	//
	// 		for (auto& [asset, rs, vc] : meshes) {
	// 			tvbs += (uint32_t) (asset->cpu.vertices.size() * sizeof(float));
	// 			tibs
	// 			  += (uint32_t) (asset->cpu.indices.size() * sizeof(uint32_t));
	//
	// 			uint32_t ts = asset->cpu.vertices.size() * sizeof(float)
	// 						+ asset->cpu.indices.size() * sizeof(uint32_t);
	// 			// PERDU_LOG_DEBUG("ii: "
	// 			// 				+ std::to_string(asset->cpu.indices.size()));
	// 			if (ts > maxsize) maxsize = ts;
	// 		}
	// 		SDL_GPUTransferBuffer* tb = get_transfer_buffer(maxsize);
	// 		uint8_t*			   mapped
	// 		  = (uint8_t*) SDL_MapGPUTransferBuffer(_ctx.device, tb, false);
	// 		// PERDU_LOG_DEBUG("maxsize: " + std::to_string(maxsize));
	//
	// 		SDL_GPUCopyPass* copy  = SDL_BeginGPUCopyPass(_precmd);
	// 		uint32_t		 vboff = 0;
	// 		uint32_t		 iboff = tvbs;
	//
	// 		for (auto& [asset, rs, vc] : meshes) {
	// 			CPUMesh& cpu = asset->cpu;
	//
	// 			uint32_t vb_size
	// 			  = static_cast<uint32_t>(cpu.vertices.size() * sizeof(float));
	// 			uint32_t ib_size = static_cast<uint32_t>(cpu.indices.size()
	// 													 * sizeof(uint32_t));
	//
	// 			// PERDU_LOG_DEBUG(std::to_string(ib_size));
	//
	// 			memcpy(mapped, cpu.vertices.data(), vb_size);
	// 			memcpy(mapped + vb_size, cpu.indices.data(), ib_size);
	//
	// 			asset->gpu.ensure_indbuf(ib_size);
	//
	// 			auto& dimbuffs = get_dim_buffers(dim, 0, vc, 0);
	// 			// PERDU_LOG_DEBUG(
	// 			//   "pos: "
	// 			//   + std::to_string(rs->voff * dim * (uint32_t)
	// 			//   sizeof(float)));
	// 			//
	// 			SDL_GPUTransferBufferLocation vbsrc{ tb, 0 };
	// 			SDL_GPUBufferRegion			  vbdst{
	// 				dimbuffs.vertex_buffer,
	// 				rs->voff * dim * (uint32_t) sizeof(float),
	// 				vb_size
	// 			};
	// 			SDL_UploadToGPUBuffer(copy, &vbsrc, &vbdst, false);
	//
	// 			SDL_GPUTransferBufferLocation ibsrc{ tb, vb_size };
	// 			SDL_GPUBufferRegion ibdst{ asset->gpu.inds, 0, ib_size };
	// 			SDL_UploadToGPUBuffer(copy, &ibsrc, &ibdst, false);
	//
	// 			vboff += vb_size;
	// 			iboff += ib_size;
	//
	// 			asset->cpu.dirty = false;
	// 		}
	//
	// 		SDL_UnmapGPUTransferBuffer(_ctx.device, tb);
	// 		SDL_EndGPUCopyPass(copy);
	// 		// SDL_SubmitGPUCommandBuffer(cmd);
	//
	// 		// PERDU_LOG_DEBUG(std::to_string(tvbs));
	//
	// 		// auto& bufs = get_dim_buffers(dim, 0, 0, 0);
	// 		// debug_read_transfer_buffer(_ctx.device, bufs.vertex_buffer,
	// 		// tvbs);
	// 	}
	// }
	//
	// void Renderer::collect_transforms() {
	// 	_scene.registry.view<Transform, TransformCache, Mesh>().each(
	// 	  [&](Transform& t, TransformCache& cache, Mesh& mesh) {
	// 		  if (t.position == cache.last_pos
	// 			  && t.rotation == cache.last_rot
	// 			  && !cache._dirty)
	// 			  return;
	// 		  auto mat	   = build_rotation_matrix(t.dim(), t.rotation);
	// 		  cache.rotmat = std::move(mat);
	// 		  // for (int i = 0; i < t.position.dim(); ++i) {
	// 		  //  PERDU_LOG_DEBUG("float["
	// 		  // 			  + std::to_string(i)
	// 		  // 			  + "] = "
	// 		  // 			  + std::to_string(t.position[i]));
	// 		  // }
	//
	// 		  cache.last_rot = t.rotation;
	// 		  cache.last_pos = t.position;
	// 		  cache._dirty	 = false;
	// 	  });
	// }
	//
	// void Renderer::compute_dispatch() {
	// 	struct DimGroup
	// 	{
	// 		std::vector<EntityInfo> entity_infos;
	// 		std::vector<float>		transform_data;
	// 		std::vector<float>		vertex_data;
	// 		CameraData				camera;
	// 		uint32_t				vtot = 0;
	// 	};
	//
	// 	std::unordered_map<uint32_t, DimGroup> groups;
	//
	// 	SDL_InsertGPUDebugLabel(_precmd, "computing");
	// 	_scene.registry.view<Mesh, RenderState, TransformCache>().each(
	// 	  [&](Mesh& mesh, RenderState& state, TransformCache& cache) {
	// 		  // if (!mesh.handle->cpu.dirty) return;
	// 		  DimGroup& dg = groups[mesh.dim];
	//
	// 		  EntityInfo info{
	// 			  .voff	  = dg.vtot,
	// 			  .vcount = state.vcount,
	// 			  .moff	  = (uint32_t) dg.transform_data.size(),
	// 			  .poff
	// 			  = (uint32_t) (dg.transform_data.size() + cache.rotmat.size()),
	// 			  .camera_dist = 2.0f,
	// 			  .dim		   = mesh.dim,
	// 		  };
	//
	// 		  dg.entity_infos.push_back(info);
	//
	// 		  dg.transform_data.insert(dg.transform_data.end(),
	// 								   cache.rotmat.begin(),
	// 								   cache.rotmat.end());
	// 		  dg.transform_data.insert(dg.transform_data.end(),
	// 								   cache.last_pos.begin(),
	// 								   cache.last_pos.end());
	// 		  dg.vtot += state.vcount;
	// 		  // mesh.handle->cpu.dirty = false;
	// 	  });
	//
	// 	if (groups.empty()) return;
	//
	// 	// SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(_ctx.device);
	// 	Transform& camt = _scene.registry.get<Transform>(view->camera);
	//
	// 	for (auto& [dim, group] : groups) {
	// 		if (group.entity_infos.empty()) continue;
	//
	// 		auto v = camt.position.extend(dim);
	//
	// 		group.camera = {
	// 			build_rotation_matrix(
	// 			  dim, camt.rotation.extend(rotation_planes(dim)), true),
	// 			{ v.begin(), v.end() }
	// 		};
	// 		auto  c = _computes->get(dim);
	// 		auto& bufs
	// 		  = get_dim_buffers(dim,
	// 							(uint32_t) group.entity_infos.size(),
	// 							group.vtot,
	// 							(uint32_t) group.transform_data.size());
	//
	// 		uint32_t entity_size
	// 		  = (uint32_t) (group.entity_infos.size() * sizeof(EntityInfo));
	// 		uint32_t transform_size
	// 		  = (uint32_t) (group.transform_data.size() * sizeof(float));
	// 		uint32_t cammatsize	 = group.camera.mat.size() * sizeof(float);
	// 		uint32_t camtransize = group.camera.tran.size() * sizeof(float);
	// 		uint32_t totsize
	// 		  = entity_size + transform_size + camtransize + cammatsize;
	//
	// 		SDL_InsertGPUDebugLabel(_precmd, "tbuf");
	// 		SDL_GPUTransferBuffer* tb = get_transfer_buffer(totsize);
	// 		uint8_t*			   mapped
	// 		  = (uint8_t*) SDL_MapGPUTransferBuffer(_ctx.device, tb, true);
	//
	// 		memcpy(mapped, group.entity_infos.data(), entity_size);
	// 		memcpy(mapped + entity_size,
	// 			   group.transform_data.data(),
	// 			   transform_size);
	// 		memcpy(mapped + entity_size + transform_size,
	// 			   group.camera.mat.data(),
	// 			   cammatsize);
	// 		memcpy(mapped + entity_size + transform_size + cammatsize,
	// 			   group.camera.tran.data(),
	// 			   camtransize);
	//
	// 		SDL_UnmapGPUTransferBuffer(_ctx.device, tb);
	//
	// 		SDL_InsertGPUDebugLabel(_precmd, "tbuffin");
	// 		SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(_precmd);
	//
	// 		SDL_GPUTransferBufferLocation entity_src{ tb, 0 };
	// 		SDL_GPUBufferRegion			  entity_dst{ bufs.entity_buffer,
	// 												  0,
	// 												  entity_size };
	// 		SDL_UploadToGPUBuffer(copy, &entity_src, &entity_dst, false);
	//
	// 		SDL_GPUTransferBufferLocation transform_src{ tb, entity_size };
	// 		SDL_GPUBufferRegion			  transform_dst{ bufs.transform_buffer,
	// 													 0,
	// 													 transform_size };
	// 		SDL_UploadToGPUBuffer(copy, &transform_src, &transform_dst, false);
	//
	// 		SDL_GPUTransferBufferLocation cam_src{
	// 			tb, entity_size + transform_size
	// 		};
	// 		SDL_GPUBufferRegion cam_dst{ bufs.cam_buffer,
	// 									 0,
	// 									 camtransize + cammatsize };
	// 		SDL_UploadToGPUBuffer(copy, &cam_src, &cam_dst, false);
	//
	// 		SDL_InsertGPUDebugLabel(_precmd, "uploadfin");
	// 		SDL_EndGPUCopyPass(copy);
	//
	// 		SDL_GPUStorageBufferReadWriteBinding output_binding{};
	// 		output_binding.buffer = bufs.output_buffer;
	//
	// 		SDL_GPUComputePass* pass = SDL_BeginGPUComputePass(
	// 		  _precmd, nullptr, 0, &output_binding, 1);
	//
	// 		c->gpu.bind(pass);
	//
	// 		SDL_GPUBuffer* storage_bufs[] = { bufs.entity_buffer,
	// 										  bufs.vertex_buffer,
	// 										  bufs.transform_buffer,
	// 										  bufs.cam_buffer };
	// 		SDL_BindGPUComputeStorageBuffers(pass, 0, storage_bufs, 4);
	//
	// 		struct PC
	// 		{
	// 			uint32_t total_vertices;
	// 			uint32_t entity_count;
	// 		} pc{ group.vtot, (uint32_t) group.entity_infos.size() };
	// 		SDL_PushGPUComputeUniformData(_precmd, 0, &pc, sizeof(pc));
	//
	// 		uint32_t groups_count = (group.vtot + 63) / 64;
	// 		SDL_DispatchGPUCompute(pass, groups_count, 1, 1);
	//
	// 		SDL_EndGPUComputePass(pass);
	// 	}
	//
	// 	SDL_SubmitGPUCommandBuffer(_precmd);
	//
	// 	// for (auto& [dim, group] : groups) {
	// 	// 	auto& bufs = get_dim_buffers(dim, 0, 0, 0);
	// 	// debug_read_transfer_buffer(_ctx.device,
	// 	// 						   bufs.vertex_buffer,
	// 	// 						   group.vertex_data.size()
	// 	// 							 * sizeof(float));
	// 	// for (int i = 0; i < group.transform_data.size(); ++i) {
	// 	// 	PERDU_LOG_DEBUG("float["
	// 	// 					+ std::to_string(i)
	// 	// 					+ "] = "
	// 	// 					+ std::to_string(group.transform_data[i]));
	// 	// }
	//
	// 	// auto&				 bufs = get_dim_buffers(dim, 0, 0, 0);
	// 	//
	// 	// std::vector<Vectorf> proj;
	// 	// read_output_buffer(
	// 	//   _ctx.device, bufs.output_buffer, group.vtot, proj);
	// 	//
	// 	// for (size_t i = 0; i < proj.size(); ++i) {
	// 	// 	PERDU_LOG_DEBUG("dim "
	// 	// 					+ std::to_string(dim)
	// 	// 					+ " vertex "
	// 	// 					+ std::to_string(i)
	// 	// 					+ ": "
	// 	// 					+ perdu::to_string(proj[i]));
	// 	// }
	// 	// }
	// }

	bool TransformCache::compare(const Transform& t) {
		return last_pos == t.position && last_rot == t.rotation;
	}
}
