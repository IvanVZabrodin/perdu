#pragma once

#include "perdu/app/application.hpp"
#include "renderer/gpu_context.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_raii.hpp"

#include <cstdint>
#include <SDL3/SDL_video.h>
#include <string>
#include <string_view>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace perdu {
	struct GPUContext
	{
		vk::raii::Context		 context;
		vk::raii::Instance		 instance		 = nullptr;
		vk::raii::PhysicalDevice physical_device = nullptr;

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
	};

	struct CommandPool
	{
		GPUContext*							 ctx  = nullptr;
		vk::raii::CommandPool				 pool = nullptr;
		std::vector<vk::raii::CommandBuffer> buffers;

		CommandPool(GPUContext*					  ctx,
					uint32_t					  buffer_count,
					vk::CommandPoolCreateFlagBits flags = {});
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
		vk::raii::Semaphore semaphore = nullptr;

		Semaphore(GPUContext* ctx) {
			semaphore
			  = vk::raii::Semaphore(ctx->device, vk::SemaphoreCreateInfo{});
		}

		vk::raii::Semaphore& operator*() { return semaphore; }
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
		GPUContext* ctx;
		uint32_t	size;

		vk::raii::Buffer	   buffer = nullptr;
		vk::raii::DeviceMemory memory = nullptr;

		static constexpr vk::MemoryPropertyFlags CPUReadable
		  = vk::MemoryPropertyFlagBits::eHostVisible
		  | vk::MemoryPropertyFlagBits::eHostCoherent;
		static constexpr vk::MemoryPropertyFlags GPULocal
		  = vk::MemoryPropertyFlagBits::eDeviceLocal;

		Buffer() {};
		Buffer(GPUContext*			   ctx,
			   uint32_t				   size,
			   vk::BufferUsageFlags	   usage	  = {},
			   vk::MemoryPropertyFlags properties = CPUReadable);

		uint32_t
			 find_memory(uint32_t filter, vk::MemoryPropertyFlags properties);
		void allocate(vk::MemoryPropertyFlags properties);

		virtual void
		  write(const void* data, uint32_t __size, uint32_t offset = 0) {
			void* bd = memory.mapMemory(offset, __size);
			memcpy(bd, data, __size);
			memory.unmapMemory();
		}

		vk::raii::Buffer& operator*() { return buffer; }
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
			data = memory.mapMemory(0, size);
		}

		virtual void
		  write(const void* __data, uint32_t __size, uint32_t offset = 0) {
			memcpy(data, __data, __size);
		}
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
