#pragma once

#include "perdu/assets/asset_cache.hpp"
#include "perdu/engine/scene.hpp"
#include "perdu/renderer/gpu_context.hpp"
#include "perdu/renderer/pipeline.hpp"

#include <cstdint>
#include <entt/entt.hpp>
#include <memory>

struct SDL_GPURenderPass;
struct SDL_GPUCommandBuffer;
struct SDL_GPUBuffer;
struct SDL_GPUTransferBuffer;
struct SDL_GPUFence;

namespace perdu {
	class PipelineCache;
	class ComputeCache;

	struct RenderOffsets
	{
		// The vertex offset - not including `sizeof(float)`
		uint32_t vert	   = 0;
		// The transform offset - not including `sizeof(float)`
		uint32_t transform = 0;
		// The entityinfo offset - not including `sizeof(EntityInfo)`
		uint32_t entity	   = 0;

		uint32_t indicies = 0;
	};

	class Renderer {
	  public:
		RenderView* view;
		uint32_t	chunk_size = 1 << 20;

		explicit Renderer(GPUContext& ctx, Scene& scene);
		~Renderer();

		Renderer(const Renderer&)			 = delete;
		Renderer& operator=(const Renderer&) = delete;

		void on_resize(uint32_t width, uint32_t height);
		void begin_frame();
		void prerender();
		void render();
		void end_frame();

		void on_mesh_construct(entt::registry& reg, entt::entity e);
		void on_mesh_destruct(entt::registry& reg, entt::entity e);

		void on_cam_construct(entt::registry& reg, entt::entity e);
		void on_cam_destruct(entt::registry& reg, entt::entity e);

		ShaderHandle vert, frag;

	  private:
		GPUContext&									_ctx;
		Scene&										_scene;
		std::unique_ptr<PipelineCache>				_pipelines;
		std::unique_ptr<ComputeCache>				_computes;
		SDL_GPUCommandBuffer*						_cmd;
		SDL_GPUCommandBuffer*						_precmd;
		SDL_GPURenderPass*							_pass;
		SDL_GPUTransferBuffer*						_transfer;
		SDL_GPUBuffer*								_inds;
		uint32_t									_isize;
		uint32_t									_tsize;
		std::unordered_map<uint32_t, RenderOffsets> _dimtooff;
		uint32_t									_indoff		 = 0;
		bool										_prev_resize = false;
		SDL_GPUFence*								_lastfence	 = nullptr;

		struct DimBuffers
		{
			SDL_GPUBuffer* entity_buffer	  = nullptr;
			SDL_GPUBuffer* vertex_buffer	  = nullptr;
			SDL_GPUBuffer* transform_buffer	  = nullptr;
			SDL_GPUBuffer* cam_buffer		  = nullptr;
			SDL_GPUBuffer* output_buffer	  = nullptr;
			uint32_t	   entity_capacity	  = 0;
			uint32_t	   vertex_capacity	  = 0;
			uint32_t	   transform_capacity = 0;
			uint32_t	   output_capacity	  = 0;
		};

		std::unordered_map<uint32_t, DimBuffers> _dim_buffers;

		SDL_GPUTransferBuffer* get_transfer_buffer(uint32_t size);
		std::pair<DimBuffers&, bool>
		  get_dim_buffers(uint32_t				dim,
						  uint32_t				entsize,
						  uint32_t				vsize,
						  uint32_t				tsize,
						  SDL_GPUCommandBuffer* cmd = nullptr);

		bool ensure_dim_buf(SDL_GPUBuffer*&		  buf,
							uint32_t&			  size,
							uint32_t			  required,
							uint32_t			  usage = (1u << 4),
							bool				  copy	= false,
							SDL_GPUCommandBuffer* cmd	= nullptr);

		RenderOffsets
		  allocate_for_dim(uint32_t dim, uint32_t size, uint32_t indsize);

		void collect_meshes();
		void collect_transforms();
		void compute_dispatch();
	};
}
