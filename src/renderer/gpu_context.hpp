#pragma once

#include "perdu/app/application.hpp"
#include "perdu/core/log.hpp"
#include "renderer/gpu_context.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_raii.hpp"

#include <cstddef>
#include <cstdint>
#include <SDL3/SDL_video.h>
#include <string>
#include <string_view>
#include <vector>
#include <vk_mem_alloc.h>

namespace perdu {
	struct GPUContext
	{
		vk::raii::Context		 context;
		vk::raii::Instance		 instance		 = nullptr;
		vk::raii::PhysicalDevice physical_device = nullptr;
		VmaAllocator			 allocator;

		vk::raii::Device device		  = nullptr;
		uint32_t		 gcqueueindex = 0;
		vk::raii::Queue	 gcq		  = nullptr;

		std::vector<const char*> requiredextensions;


		GPUContext(std::string				appname,
				   AppVersion				version,
				   std::vector<const char*> requiredextensions
				   = { vk::KHRSwapchainExtensionName });

		void pick_physical_device();
		int	 device_suitability(const vk::raii::PhysicalDevice& dev);
		void create_logical_device();
		void create_allocator();

		~GPUContext() { vmaDestroyAllocator(allocator); }
	};

	struct CommandPool
	{
		GPUContext*							 ctx  = nullptr;
		vk::raii::CommandPool				 pool = nullptr;
		std::vector<vk::raii::CommandBuffer> buffers;

		CommandPool(GPUContext*				   ctx,
					uint32_t				   buffer_count,
					vk::CommandPoolCreateFlags flags
					= vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
		std::vector<vk::raii::CommandBuffer> quick_create(uint32_t count);
	};


	struct WinContext
	{
		GPUContext*			 ctx = nullptr;
		SDL_Window*			 window;
		vk::raii::SurfaceKHR surface = nullptr;

		WinContext(std::string title, int w, int h, SDL_WindowFlags flags);

		void create_surface(GPUContext* ctx);
	};

	struct RenderTarget
	{
		vk::Viewport viewport;
		vk::Rect2D	 scissor;
	};

	struct Swapchain
	{
		WinContext*						 wtx = nullptr;
		CommandPool*					 cmd = nullptr;
		vk::raii::SwapchainKHR			 swp = nullptr;
		std::vector<vk::Image>			 images;
		std::vector<vk::raii::ImageView> views;
		vk::Extent2D					 extent;
		vk::SurfaceFormatKHR			 format;

		Swapchain(WinContext* wtx, CommandPool* cmd, bool vsync = false);
		~Swapchain();

		void transition_layout(uint32_t				   cmdidx,
							   uint32_t				   index,
							   vk::ImageLayout		   oldlayout,
							   vk::ImageLayout		   newlayout,
							   vk::AccessFlags2		   src_access,
							   vk::AccessFlags2		   dst_access,
							   vk::PipelineStageFlags2 src_stage,
							   vk::PipelineStageFlags2 dst_stage);

		void create_image_views();

		vk::Extent2D
		  choose_extent(const vk::SurfaceCapabilitiesKHR& capabilities);
		vk::PresentModeKHR
		  choose_present_mode(const std::vector<vk::PresentModeKHR>& available,
							  bool vsync = false);

		vk::SurfaceFormatKHR choose_surface_format(
		  const std::vector<vk::SurfaceFormatKHR>& available);

		uint32_t
		  choose_minimage_count(const vk::SurfaceCapabilitiesKHR& capabilities);
	};

	struct Semaphore
	{
		GPUContext*			ctx;
		vk::raii::Semaphore semaphore = nullptr;
		bool				timeline;

		Semaphore(GPUContext* __ctx,
				  bool		  __timeline = false,
				  uint64_t	  initial	 = 0) :
			ctx(__ctx), timeline(__timeline) {
			vk::SemaphoreTypeCreateInfo typeinfo{
				.semaphoreType = __timeline ? vk::SemaphoreType::eTimeline
											: vk::SemaphoreType::eBinary,
				.initialValue  = initial
			};

			vk::SemaphoreCreateInfo cinfo{
				.sType = vk::StructureType::eSemaphoreCreateInfo,
				.pNext = &typeinfo
			};

			semaphore = vk::raii::Semaphore(ctx->device, cinfo);
		}

		vk::raii::Semaphore& operator*() { return semaphore; }
	};

	struct TimelineSemaphore : public Semaphore
	{
		TimelineSemaphore(GPUContext* ctx, uint64_t initial = 0) :
			Semaphore(ctx, true, initial) {}

		uint64_t   get_value() const { return semaphore.getCounterValue(); }
		vk::Result wait(uint64_t target_val, uint64_t timeout = UINT64_MAX) {
			vk::SemaphoreWaitInfo winfo{ .semaphoreCount = 1,
										 .pSemaphores	 = &(*semaphore),
										 .pValues		 = &target_val };

			return ctx->device.waitSemaphores(winfo, timeout);
		}

		vk::SemaphoreSubmitInfo
		  gpu_wait(uint64_t				   target_val,
				   vk::PipelineStageFlags2 mask
				   = vk::PipelineStageFlagBits2::eAllCommands) const {
			return { .semaphore = *semaphore,
					 .value		= target_val,
					 .stageMask = mask };
		}
	};

	struct Fence
	{
		vk::raii::Fence fence = nullptr;

		Fence(GPUContext* ctx) {
			fence = vk::raii::Fence(
			  ctx->device, { .flags = vk::FenceCreateFlagBits::eSignaled });
		}

		vk::raii::Fence& operator*() { return fence; }
	};

	struct Buffer
	{
		GPUContext*		  ctx;
		VmaAllocation	  alloc;
		VmaAllocationInfo info;
		uint32_t		  size;
		bool			  coherent = false;

		vk::Buffer buffer = nullptr;

		static constexpr vk::MemoryPropertyFlags CPUReadable
		  = vk::MemoryPropertyFlagBits::eHostVisible
		  | vk::MemoryPropertyFlagBits::eHostCoherent;
		static constexpr vk::MemoryPropertyFlags GPULocal
		  = vk::MemoryPropertyFlagBits::eDeviceLocal;

		Buffer() {};
		Buffer(GPUContext*				ctx,
			   uint32_t					size,
			   vk::BufferUsageFlags		usage	   = {},
			   vk::MemoryPropertyFlags	properties = CPUReadable,
			   VmaAllocationCreateFlags allocation
			   = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
			   VmaMemoryUsage memusage = VMA_MEMORY_USAGE_AUTO);
		virtual ~Buffer();

		virtual void
		  write(const void* data, uint32_t __size, uint32_t offset = 0) {
			void* mapped;
			if (vmaMapMemory(ctx->allocator, alloc, &mapped) != VK_SUCCESS) {
				PERDU_LOG_ERROR("failed to map buffer memory");
				return;
			}
			auto* moff = static_cast<std::byte*>(mapped) + offset;
			memcpy(moff, data, __size);

			flush_if_needed(__size, offset);

			vmaUnmapMemory(ctx->allocator, alloc);
		}

		virtual inline void
		  flush_if_needed(uint32_t __size, uint32_t __offset) {
			if (!coherent)
				vmaFlushAllocation(ctx->allocator, alloc, __offset, __size);
		}

		vk::Buffer& operator*() { return buffer; }
	};

	struct UniformBuffer : public Buffer
	{
		void* data;

		UniformBuffer(GPUContext*			  ctx,
					  uint32_t				  size,
					  vk::BufferUsageFlags	  usage		 = {},
					  vk::MemoryPropertyFlags properties = CPUReadable) :
			Buffer(ctx,
				   size,
				   vk::BufferUsageFlagBits::eUniformBuffer | usage,
				   properties) {
			if (vmaMapMemory(ctx->allocator, alloc, &data) != VK_SUCCESS) {
				PERDU_LOG_ERROR("failed to map uniform buffer memory");
				return;
			}
		}

		virtual ~UniformBuffer();

		virtual void
		  write(const void* __data, uint32_t __size, uint32_t offset = 0) {
			auto* moff = static_cast<std::byte*>(data) + offset;
			memcpy(moff, __data, __size);
			flush_if_needed(__size, offset);
		}
	};

	// The following things need to be supported via the SSBO:
	// - ring buffer (either by renderer or by the buffer itself)
	// - copy from staging
	// - write straight via rebar or just CPUReadable
	// struct SSBO : public Buffer
	// {
	// 	SSBO(GPUContext*			 ctx,
	// 		 uint32_t				 size,
	// 		 vk::BufferUsageFlags	 usage		= {},
	// 		 vk::MemoryPropertyFlags properties = CPUReadable) :
	// 		Buffer(ctx, size, usage, properties) {}
	// };

	// staging ring buffer
	struct SRB : public Buffer
	{
		uint8_t* base;
		uint32_t region_size;
		uint32_t head  = 0;
		uint32_t frame = 0;

		struct Slice
		{
			void*		 ptr;
			VkDeviceSize offset;
		};

		SRB(GPUContext* ctx, uint32_t size) :
			Buffer(ctx,
				   size,
				   vk::BufferUsageFlagBits::eTransferSrc,
				   {},
				   VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
					 | VMA_ALLOCATION_CREATE_MAPPED_BIT
					 | VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
				   VMA_MEMORY_USAGE_AUTO) {
			base = static_cast<uint8_t*>(info.pMappedData);
		}

		void begin_frame(uint32_t __frame) {
			frame = __frame;
			head  = 0;
		}

		Slice allocate(uint32_t size, uint32_t align = 16);

		virtual void
		  write(const void* __data, uint32_t __size, uint32_t offset) {
			memcpy(base + offset, __data, __size);
		}

		virtual void flush_if_needed(uint32_t size, uint32_t offset);
	};

	struct DescriptorPool
	{
		GPUContext* ctx;

		vk::raii::DescriptorPool			 pool = nullptr;
		std::vector<vk::raii::DescriptorSet> sets;

		DescriptorPool(GPUContext*							ctx,
					   std::vector<vk::DescriptorSetLayout> layouts);
	};

	Buffer create_staging_buffer(GPUContext*		  ctx,
								 uint32_t			  size,
								 bool				  staging = true,
								 vk::BufferUsageFlags usage	  = {});

	void copy_buffer(CommandPool& cmdpool,
					 Buffer&	  src,
					 Buffer&	  dst,
					 uint32_t	  size);
};
