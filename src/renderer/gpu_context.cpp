#include "perdu/renderer/gpu_context.hpp"

#include "perdu/core/assert.hpp"
#include "perdu/core/log.hpp"
#include "renderer/gpu_context.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>
#include <string_view>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>


const std::vector<const char*> validationLayers
  = { "VK_LAYER_KHRONOS_validation" };

static bool sdlinit = false;

namespace perdu {
	GPUContext::GPUContext(std::string				appname,
						   AppVersion				version,
						   std::vector<const char*> _requiredextensions) :
		requiredextensions(_requiredextensions) {
		if (!sdlinit) {
			SDL_Init(SDL_INIT_VIDEO);
			sdlinit = true;
		}

		PERDU_LOG_INFO("Using vulkan version "
					   + std::to_string(VK_HEADER_VERSION));

		vk::ApplicationInfo appinfo{ .pApplicationName	 = appname.c_str(),
									 .applicationVersion = VK_MAKE_VERSION(
									   version.a, version.b, version.c),
									 .pEngineName	= "perdu",
									 .engineVersion = VK_MAKE_VERSION(0, 2, 0),
									 .apiVersion	= vk::ApiVersion13 };

		uint32_t sdlextcount   = 0;
		auto	 sdlextensions = SDL_Vulkan_GetInstanceExtensions(&sdlextcount);
		PERDU_LOG_INFO(SDL_GetError());

		auto extprops = context.enumerateInstanceExtensionProperties();
		for (uint32_t i = 0; i < sdlextcount; ++i) {
			if (std::ranges::none_of(
				  extprops,
				  [sdlext = sdlextensions[i]](const auto& extprop) {
					  return strcmp(extprop.extensionName, sdlext) == 0;
				  }))
			{
				PERDU_LOG_ERROR("Required SDL extension not supported: "
								+ std::string(sdlextensions[i]));
				return;
			}
		}


		std::vector<const char*> requiredLayers;

#ifndef NDEBUG
		requiredLayers.assign(validationLayers.begin(), validationLayers.end());
#endif
		auto layerProperties	= context.enumerateInstanceLayerProperties();
		auto unsupportedLayerIt = std::ranges::find_if(
		  requiredLayers, [&layerProperties](const auto& requiredLayer) {
			  return std::ranges::none_of(
				layerProperties, [requiredLayer](const auto& layerProperty) {
					return strcmp(layerProperty.layerName, requiredLayer) == 0;
				});
		  });
		if (unsupportedLayerIt != requiredLayers.end()) {
			PERDU_LOG_ERROR("Required layer not supported: "
							+ std::string(*unsupportedLayerIt));
			return;
		}

		vk::InstanceCreateInfo createinfo{
			.pApplicationInfo	 = &appinfo,
			.enabledLayerCount	 = static_cast<uint32_t>(requiredLayers.size()),
			.ppEnabledLayerNames = requiredLayers.data(),
			.enabledExtensionCount	 = sdlextcount,
			.ppEnabledExtensionNames = sdlextensions,
		};

		instance = vk::raii::Instance(context, createinfo);
	}

	void GPUContext::pick_physical_device() {
		auto devs = instance.enumeratePhysicalDevices();
		if (devs.empty()) {
			PERDU_LOG_ERROR("No physical devices found");
			return;
		}

		vk::raii::PhysicalDevice best		= nullptr;
		int						 best_score = -1;

		for (auto& dev : devs) {
			int score = device_suitability(dev);
			if (score > best_score) {
				best_score = score;
				best	   = dev;
			}
		}

		if (best_score == -1) {
			PERDU_LOG_ERROR("No suitable devices found");
			return;
		}

		physical_device = best;
	}

	int GPUContext::device_suitability(const vk::raii::PhysicalDevice& dev) {
		auto devprops = dev.getProperties();
		auto devfeats = dev.getFeatures();

		auto queuefamilies = dev.getQueueFamilyProperties();
		auto requiredqueues
		  = { vk::QueueFlagBits::eGraphics, vk::QueueFlagBits::eCompute };
		bool supports_queues = std::ranges::all_of(
		  requiredqueues, [&queuefamilies](const auto& req) {
			  return std::ranges::any_of(queuefamilies, [req](const auto& qfp) {
				  return !!(qfp.queueFlags & req);
			  });
		  });
		if (!supports_queues) return -1; // TODO: Add compute check

		auto devexts	   = dev.enumerateDeviceExtensionProperties();
		bool supports_exts = std::ranges::all_of(
		  requiredextensions, [&devexts](const auto& devext) {
			  return std::ranges::any_of(devexts, [devext](const auto& avext) {
				  return strcmp(avext.extensionName, devext) == 0;
			  });
		  });
		if (!supports_exts) return -1;


		if (devprops.apiVersion < vk::ApiVersion13) return -1;

		int score = 0;
		if (devprops.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
			score += 1000;
		else if (devprops.deviceType == vk::PhysicalDeviceType::eIntegratedGpu)
			score += 100;


		return score;
	}

	void GPUContext::create_logical_device() {
		std::vector<vk::QueueFamilyProperties> qfps
		  = physical_device.getQueueFamilyProperties();

		uint32_t queueIndex = ~0;
		PERDU_LOG_INFO(std::to_string(qfps.size()));
		for (uint32_t qfpIndex = 0; qfpIndex < qfps.size(); qfpIndex++) {
			PERDU_LOG_INFO("instance: "
						   + std::to_string((size_t) (VkInstance) *instance));
			PERDU_LOG_INFO(
			  "physdev:  "
			  + std::to_string((size_t) (VkPhysicalDevice) *physical_device));
			if ((qfps[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics)
				&& (qfps[qfpIndex].queueFlags & vk::QueueFlagBits::eCompute)
				&& SDL_Vulkan_GetPresentationSupport(
				  *instance, *physical_device, qfpIndex))
			{
				// found a queue family that supports both graphics and present
				queueIndex = qfpIndex;
				break;
			}
			PERDU_LOG_INFO(SDL_GetError());
		}
		if (queueIndex == ~0) {
			PERDU_LOG_ERROR(
			  "Failed to find queue for graphics, present and compute");
			return;
		}
		gcqueueindex = queueIndex;

		float					  qpriority = 0.5f;
		vk::DeviceQueueCreateInfo devqinfo{ .queueFamilyIndex = gcqueueindex,
											.queueCount		  = 1,
											.pQueuePriorities = &qpriority };

		vk::StructureChain<vk::PhysicalDeviceFeatures2,
						   vk::PhysicalDeviceVulkan13Features,
						   vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
		  featureChain = {
			  {}, // vk::PhysicalDeviceFeatures2 (empty for now)
			  { .synchronization2 = true,
				.dynamicRendering
				= true }, // Enable dynamic rendering from Vulkan 1.3
			  { .extendedDynamicState
				= true }  // Enable extended dynamic state from the extension
		  };


		vk::DeviceCreateInfo devcinfo{
			.pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
			.queueCreateInfoCount = 1,
			.pQueueCreateInfos	  = &devqinfo,
			.enabledExtensionCount
			= static_cast<uint32_t>(requiredextensions.size()),
			.ppEnabledExtensionNames = requiredextensions.data()
		};

		device = vk::raii::Device(physical_device, devcinfo);

		gcq = vk::raii::Queue(device, gcqueueindex, 0);
	}

	WinContext::WinContext(std::string	   title,
						   int			   w,
						   int			   h,
						   SDL_WindowFlags flags) {
		if (!sdlinit) {
			SDL_Init(SDL_INIT_VIDEO);
			sdlinit = true;
		}
		window
		  = SDL_CreateWindow(title.c_str(), w, h, flags | SDL_WINDOW_VULKAN);
	}

	void WinContext::create_surface(GPUContext* _ctx) {
		ctx = _ctx;
		VkSurfaceKHR _surface;
		if (!SDL_Vulkan_CreateSurface(
			  window, *ctx->instance, nullptr, &_surface))
		{
			PERDU_LOG_ERROR("failed to create surface");
			return;
		}
		surface = vk::raii::SurfaceKHR(ctx->instance, _surface);
	}

	Swapchain::Swapchain(WinContext* __wtx, CommandPool* __cmd) :
		wtx(__wtx), cmd(__cmd) {
		vk::SurfaceCapabilitiesKHR scap
		  = wtx->ctx->physical_device.getSurfaceCapabilitiesKHR(*wtx->surface);
		extent				   = choose_extent(scap);
		uint32_t minimagecount = choose_minimage_count(scap);

		std::vector<vk::SurfaceFormatKHR> available
		  = wtx->ctx->physical_device.getSurfaceFormatsKHR(*wtx->surface);
		format = choose_surface_format(available);

		std::vector<vk::PresentModeKHR> availablepresent
		  = wtx->ctx->physical_device.getSurfacePresentModesKHR(*wtx->surface);

		vk::SwapchainCreateInfoKHR createinfo{
			.surface		  = *wtx->surface,
			.minImageCount	  = minimagecount,
			.imageFormat	  = format.format,
			.imageColorSpace  = format.colorSpace,
			.imageExtent	  = extent,
			.imageArrayLayers = 1,
			.imageUsage		  = vk::ImageUsageFlagBits::eColorAttachment,
			.imageSharingMode = vk::SharingMode::eExclusive,
			.preTransform	  = scap.currentTransform,
			.compositeAlpha	  = vk::CompositeAlphaFlagBitsKHR::eOpaque,
			.presentMode	  = choose_present_mode(availablepresent),
			.clipped		  = true
		};

		swp	   = vk::raii::SwapchainKHR(wtx->ctx->device, createinfo);
		images = swp.getImages();
		create_image_views();
	}

	vk::SurfaceFormatKHR Swapchain::choose_surface_format(
	  const std::vector<vk::SurfaceFormatKHR>& available) {
		const auto formatIt
		  = std::ranges::find_if(available, [](const auto& format) {
				return format.format == vk::Format::eB8G8R8A8Srgb
					&& format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
			});
		return formatIt != available.end() ? *formatIt : available[0];
	}

	vk::PresentModeKHR Swapchain::choose_present_mode(
	  const std::vector<vk::PresentModeKHR>& available) {
		return std::ranges::any_of(available,
								   [](const vk::PresentModeKHR value) {
									   return vk::PresentModeKHR::eMailbox
										   == value;
								   })
			   ? vk::PresentModeKHR::eMailbox
			   : vk::PresentModeKHR::eFifo;
	}

	vk::Extent2D
	  Swapchain::choose_extent(const vk::SurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width
			!= std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}
		int width, height;
		SDL_GetWindowSizeInPixels(wtx->window, &width, &height);

		return { std::clamp<uint32_t>(width,
									  capabilities.minImageExtent.width,
									  capabilities.maxImageExtent.width),
				 std::clamp<uint32_t>(height,
									  capabilities.minImageExtent.height,
									  capabilities.maxImageExtent.height) };
	}


	uint32_t Swapchain::choose_minimage_count(
	  const vk::SurfaceCapabilitiesKHR& capabilities) {
		auto minImageCount = std::max(3u, capabilities.minImageCount);
		if ((0 < capabilities.maxImageCount)
			&& (capabilities.maxImageCount < minImageCount))
		{
			minImageCount = capabilities.maxImageCount;
		}
		return minImageCount;
	}

	void Swapchain::create_image_views() {
		PERDU_ASSERT(views.empty(), "image views are already created");

		vk::ImageViewCreateInfo createinfo{
			.viewType		  = vk::ImageViewType::e2D,
			.format			  = format.format,
			.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
		};

		for (auto& image : images) {
			createinfo.image = image;
			views.emplace_back(wtx->ctx->device, createinfo);
		}
	}

	void Swapchain::transition_layout(uint32_t				  index,
									  vk::ImageLayout		  oldlayout,
									  vk::ImageLayout		  newlayout,
									  vk::AccessFlags2		  src_access,
									  vk::AccessFlags2		  dst_access,
									  vk::PipelineStageFlags2 src_stage,
									  vk::PipelineStageFlags2 dst_stage) {
		vk::ImageMemoryBarrier2 barrier = {
			.srcStageMask		 = src_stage,
			.srcAccessMask		 = src_access,
			.dstStageMask		 = dst_stage,
			.dstAccessMask		 = dst_access,
			.oldLayout			 = oldlayout,
			.newLayout			 = newlayout,
			.srcQueueFamilyIndex = vk::QueueFamilyIgnored,
			.dstQueueFamilyIndex = vk::QueueFamilyIgnored,
			.image				 = images[index],
			.subresourceRange = { .aspectMask = vk::ImageAspectFlagBits::eColor,
								  .baseMipLevel	  = 0,
								  .levelCount	  = 1,
								  .baseArrayLayer = 0,
								  .layerCount	  = 1 }
		};

		vk::DependencyInfo depinfo = { .dependencyFlags			= {},
									   .imageMemoryBarrierCount = 1,
									   .pImageMemoryBarriers	= &barrier };
		cmd->buffers[0].pipelineBarrier2(depinfo);
	}


	CommandPool::CommandPool(GPUContext* __ctx, uint32_t bufcount) :
		ctx(__ctx) {
		vk::CommandPoolCreateInfo poolinfo{
			.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
			.queueFamilyIndex = ctx->gcqueueindex
		};

		pool = vk::raii::CommandPool(ctx->device, poolinfo);

		vk::CommandBufferAllocateInfo allocinfo{
			.commandPool		= *pool,
			.level				= vk::CommandBufferLevel::ePrimary,
			.commandBufferCount = bufcount
		};

		buffers = vk::raii::CommandBuffers(ctx->device, allocinfo);
	}
}
