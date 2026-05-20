#pragma once

#include "perdu/app/application.hpp"
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

		CommandPool(GPUContext* ctx, uint32_t buffer_count);
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

		Swapchain(WinContext* wtx, CommandPool* cmd);

		void transition_layout(uint32_t				   index,
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
		  choose_present_mode(const std::vector<vk::PresentModeKHR>& available);

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
			  = vk::raii::Semaphore(ctx->device, vk::SemaphoreCreateInfo());
		}
	};

	struct Fence
	{
		vk::raii::Fence fence = nullptr;

		Fence(GPUContext* ctx) {
			fence = vk::raii::Fence(
			  ctx->device, { .flags = vk::FenceCreateFlagBits::eSignaled });
		}
	};


}
