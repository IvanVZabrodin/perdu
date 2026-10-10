#pragma once

#include "perdu/assets/asset_cache.hpp"
// #include "perdu/components/material.hpp"
#include "perdu/core/maths.hpp"
#include "perdu/engine/scene.hpp"
// #include "perdu/renderer/area_manager.hpp"
#include "perdu/renderer/gpu_context.hpp"
#include "perdu/renderer/mesh.hpp"
#include "perdu/renderer/pipeline.hpp"

#include <cstdint>
#include <entt/entt.hpp>
#include <memory>
#include <unordered_map>

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
	struct DimBuffers;
	class PipelineCache;
	class ComputeCache;
	class Pipeline;
	class UploadManager;

	struct UBO
	{
		std::array<float, 4 * 4> model;
		std::array<float, 4 * 4> view;
		std::array<float, 4 * 4> proj;
	};

	using BatchKey = std::tuple<uint32_t, uint32_t, PrimitiveType, uint32_t>;
	struct RenderOffsets
	{
		struct IndOff
		{
			BatchKey key;
			uint32_t off;
		};
		// The vertex offset - not including `sizeof(float)`
		uint32_t vert	   = 0;
		// The transform offset - not including `sizeof(float)`
		uint32_t transform = 0;
		// The entityinfo offset - not including `sizeof(EntityInfo)`
		uint32_t entity	   = 0;

		IndOff indicies = {};
	};

	class Renderer {
	  public:
		static constexpr int max_flight_frames = 3;
		RenderView*			 view;
		uint32_t			 chunk_size	  = 1 << 5;
		bool				 reload_force = false;

		explicit Renderer(GPUContext* ctx,
						  Scene&	  scene,
						  WinContext* wtx = nullptr);
		~Renderer();

		void set_wtx(WinContext* wtx);

		void make_pipeline();

		Renderer(const Renderer&)			 = delete;
		Renderer& operator=(const Renderer&) = delete;

		void on_resize(uint32_t width, uint32_t height);

		void rendertest(uint32_t ind);
		void draw();

		// void begin_frame();
		// void prerender();
		// void render();
		// void end_frame();

		void on_mesh_construct(entt::registry& reg, entt::entity e);
		void on_mesh_destruct(entt::registry& reg, entt::entity e);

		void on_cam_construct(entt::registry& reg, entt::entity e);
		void on_cam_destruct(entt::registry& reg, entt::entity e);

		void on_material_construct(entt::registry& reg, entt::entity e);
		void on_material_destruct(entt::registry& reg, entt::entity e);

		ShaderHandle vert, frag;

	  private:
		GPUContext*									_ctx;
		WinContext*									_wtx;
		Scene&										_scene;
		std::unique_ptr<CommandPool>				_cmdpool;
		std::unique_ptr<DescriptorPool>				_descpool;
		std::unique_ptr<Pipeline>					_testpipe;
		std::unique_ptr<Swapchain>					_swp;
		std::unique_ptr<Semaphore>					_presentsem;
		std::unique_ptr<Semaphore>					_rendersem;
		std::unique_ptr<Fence>						_drawfence;
		std::unique_ptr<UploadManager>				_srbman;
		std::vector<std::unique_ptr<Semaphore>>		_presentsems;
		std::vector<std::unique_ptr<Semaphore>>		_rendersems;
		std::vector<std::unique_ptr<Fence>>			_fences;
		std::unique_ptr<Buffer>						_vertbuf;
		std::unique_ptr<Buffer>						_indbuf;
		std::unique_ptr<Buffer>						_vertstage;
		std::unique_ptr<Buffer>						_indstage;
		std::vector<std::unique_ptr<UniformBuffer>> _ubos;
		uint32_t									_frameidx = 0;
		// std::unique_ptr<PipelineCache> _pipelines;
		// std::unique_ptr<ComputeCache>  _computes;
		// SDL_GPUCommandBuffer*						_cmd;
		// SDL_GPUCommandBuffer*						_precmd;
		// SDL_GPURenderPass*							_pass;
		// SDL_GPUTransferBuffer*						_transfer;
		// SDL_GPUBuffer*								_inds;
		// uint32_t									_isize;
		// uint32_t									_tsize;
		// std::unordered_map<uint32_t, RenderOffsets> _dimtooff;
		// uint32_t									_indoff		 = 0;
		// bool										_prev_resize = false;
		// SDL_GPUFence*								_lastfence	 = nullptr;
		// SDL_GPUFence*								_renderfence = nullptr;
		// uint32_t									_batchoff	 = 0;
		// uint32_t									_batchchunk	 = 1 << 5;
		// AreaManager									_indmanager{
		// _batchchunk
		// };

		void recreate_swp();
		void update_ubo(uint32_t image);

		struct Vertex
		{
			std::array<float, 2> pos;
			std::array<float, 3> colour;
		};

		const std::vector<Vertex> _verts = {
			{ { -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
			{ { 0.5f, -0.5f },  { 0.0f, 1.0f, 0.0f } },
			{ { 0.5f, 0.5f },	  { 0.0f, 0.0f, 1.0f } },
			{ { -0.5f, 0.5f },  { 1.0f, 1.0f, 1.0f } }
		};

		const std::vector<uint16_t> _inds = { 0, 1, 2, 2, 3, 0 };

		std::vector<std::tuple<uint32_t, uint32_t, uint32_t>> _indcopies;

		struct AllocBatch
		{
			BatchKey key;
			uint32_t offset;
			uint32_t count;
			uint32_t size;
		};

		struct FrameRescources
		{
			std::unique_ptr<CommandPool> cmdpool;
		};

		std::unordered_map<BatchKey, AllocBatch, TupleHash> _indbatches;
		// std::unordered_map<BatchKey, AreaManager::HandleType, TupleHash>
		// _indareas;



		std::unordered_map<uint32_t, DimBuffers>	   _dim_buffers;
		std::array<FrameRescources, max_flight_frames> _frameresc;

		DimBuffers* get_dim_buffers(uint32_t dim,
									uint32_t mesh_count,
									uint32_t entity_count);

		bool ensure_dim_buf(Buffer* buf, uint32_t size, bool copy = false);

		RenderOffsets allocate_for_dim(uint32_t		 dim,
									   uint32_t		 size,
									   uint32_t		 indsize,
									   PrimitiveType pt,
									   bool			 hasm);
		//
		// RenderOffsets::IndOff create_ind_off(uint32_t		 size,
		// 									 const Material& mat,
		// 									 PrimitiveType	 pt,
		// 									 uint32_t		 dim);

		void collect_meshes();
		void collect_transforms();
		void compute_dispatch();
	};
}
